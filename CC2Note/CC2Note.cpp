#include "AUInstrumentBase.h"
#include "CC2NoteVersion.h"
#include "MIDIOutputCallbackHelper.h"
#include <CoreMIDI/CoreMIDI.h>
#include <list>
#include <set>
#include <algorithm>

#ifdef DEBUG
#include <fstream>
#include <ctime>
#define DEBUGLOG_B(x) \
if (baseDebugFile.is_open()) baseDebugFile << x
#else
#define DEBUGLOG_B(x)
#endif

#define kNoteOn 0x90
#define kNoteOff 0x80
#define kControlChange 0xb0

using namespace std;

class CC2Note : public AUMonotimbralInstrumentBase {
public:
    CC2Note(AudioUnit inComponentInstance);
    ~CC2Note();
    
    OSStatus GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope,
                             AudioUnitElement inElement, UInt32 &outDataSize,
                             Boolean &outWritable);
    
    OSStatus GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                         AudioUnitElement inElement, void *outData);
    
    OSStatus SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope,
                         AudioUnitElement inElement, const void *inData,
                         UInt32 inDataSize);
    
    OSStatus HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1,
                             UInt8 data2, UInt32 inStartFrame);
    
    OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags,
                    const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames);
    
    OSStatus Initialize();
    void Cleanup();
    OSStatus Version() { return kCC2NoteVersion; }
    
    AUElement *CreateElement(AudioUnitScope scope, AudioUnitElement element);
    
    OSStatus GetParameterInfo(AudioUnitScope inScope,
                              AudioUnitParameterID inParameterID,
                              AudioUnitParameterInfo &outParameterInfo);
    
private:
    MIDIOutputCallbackHelper mCallbackHelper;
    int noteNumber = 0;
    bool isNoteOff = true;
    
protected:
#ifdef DEBUG
    ofstream baseDebugFile;
#endif
};

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma mark CC2Note Methods
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, CC2Note)


enum {
    kParameter_Ch = 0,
    kParameter_CC = 1,
    kParameter_NoteMin = 2,
    kParameter_NoteMax = 3,
    kParameter_Velocity = 4,
    kNumberOfParameters = 5
};

static const CFStringRef kParamName_Ch = CFSTR("Ch: ");
static const CFStringRef kParamName_CC = CFSTR("CC: ");
static const CFStringRef kParamName_NoteMin = CFSTR("Note Min: ");
static const CFStringRef kParamName_NoteMax = CFSTR("Note Max: ");
static const CFStringRef kParamName_Velocity = CFSTR("Velocity: ");

CC2Note::CC2Note(AudioComponentInstance inComponentInstance)
: AUMonotimbralInstrumentBase(inComponentInstance, 0, 1) {
    CreateElements();
    
    Globals()->UseIndexedParameters(kNumberOfParameters);
    
    Globals()->SetParameter(kParameter_Ch, 1);
    Globals()->SetParameter(kParameter_CC, 64);
    Globals()->SetParameter(kParameter_NoteMin, 60);
    Globals()->SetParameter(kParameter_NoteMax, 72);
    Globals()->SetParameter(kParameter_Velocity, 127);
    
#ifdef DEBUG
    string bPath, bFullFileName;
    bPath = getenv("HOME");
    if (!bPath.empty()) {
        bFullFileName = bPath + "/Desktop/" + "Debug.log";
    } else {
        bFullFileName = "Debug.log";
    }
    
    baseDebugFile.open(bFullFileName.c_str());
    DEBUGLOG_B("Plug-in constructor invoked with parameters:" << endl);
#endif
}

CC2Note::~CC2Note() {
#ifdef DEBUG
    DEBUGLOG_B("CC2Note::~CC2Note" << endl);
#endif
}

OSStatus CC2Note::GetPropertyInfo(AudioUnitPropertyID inID,
                                       AudioUnitScope inScope,
                                       AudioUnitElement inElement,
                                       UInt32 &outDataSize,
                                       Boolean &outWritable) {
    if (inScope == kAudioUnitScope_Global) {
        if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
            outDataSize = sizeof(CFArrayRef);
            outWritable = false;
            return noErr;
        } else if (inID == kAudioUnitProperty_MIDIOutputCallback) {
            outDataSize = sizeof(AUMIDIOutputCallbackStruct);
            outWritable = true;
            return noErr;
        }
    }
    return AUMonotimbralInstrumentBase::GetPropertyInfo(inID, inScope, inElement,
                                                        outDataSize, outWritable);
}

void CC2Note::Cleanup() {
#ifdef DEBUG
    DEBUGLOG_B("CC2Note::Cleanup" << endl);
#endif
}

OSStatus CC2Note::Initialize() {
#ifdef DEBUG
    DEBUGLOG_B("->CC2Note::Initialize" << endl);
#endif
    
    AUMonotimbralInstrumentBase::Initialize();
    
#ifdef DEBUG
    DEBUGLOG_B("<-CC2Note::Initialize" << endl);
#endif
    
    return noErr;
}

AUElement *CC2Note::CreateElement(AudioUnitScope scope,
                                       AudioUnitElement element) {
#ifdef DEBUG
    DEBUGLOG_B("CreateElement - scope: " << scope << endl);
#endif
    switch (scope) {
        case kAudioUnitScope_Group:
            return new SynthGroupElement(this, element, new MidiControls);
        case kAudioUnitScope_Part:
            return new SynthPartElement(this, element);
        default:
            return AUBase::CreateElement(scope, element);
    }
}

OSStatus CC2Note::GetParameterInfo(
                                        AudioUnitScope inScope, AudioUnitParameterID inParameterID,
                                        AudioUnitParameterInfo &outParameterInfo) {
    
#ifdef DEBUG
    DEBUGLOG_B("GetParameterInfo - inScope: " << inScope << endl);
    DEBUGLOG_B("GetParameterInfo - inParameterID: " << inParameterID << endl);
#endif
    
    if (inScope != kAudioUnitScope_Global) return kAudioUnitErr_InvalidScope;
    
    outParameterInfo.flags += kAudioUnitParameterFlag_IsWritable;
    outParameterInfo.flags += kAudioUnitParameterFlag_IsReadable;
    
    switch (inParameterID) {
        case kParameter_Ch:
            AUBase::FillInParameterName(outParameterInfo, kParamName_Ch, false);
            outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
            outParameterInfo.minValue = 1;
            outParameterInfo.maxValue = 16;
            break;
        case kParameter_CC:
            AUBase::FillInParameterName(outParameterInfo, kParamName_CC,
                                        false);
            outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
            outParameterInfo.minValue = 1;
            outParameterInfo.maxValue = 127;
            break;
        case kParameter_NoteMin:
            AUBase::FillInParameterName(outParameterInfo, kParamName_NoteMin,
                                        false);
            outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
            outParameterInfo.minValue = 1;
            outParameterInfo.maxValue = 127;
            break;
        case kParameter_NoteMax:
            AUBase::FillInParameterName(outParameterInfo, kParamName_NoteMax,
                                        false);
            outParameterInfo.unit = kAudioUnitParameterUnit_MIDINoteNumber;
            outParameterInfo.minValue = 1;
            outParameterInfo.maxValue = 127;
            break;
        case kParameter_Velocity:
            AUBase::FillInParameterName(outParameterInfo, kParamName_Velocity,
                                        false);
            outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
            outParameterInfo.minValue = 1;
            outParameterInfo.maxValue = 127;
            break;
        default:
            return kAudioUnitErr_InvalidParameter;
    }

    return noErr;
}

OSStatus CC2Note::GetProperty(AudioUnitPropertyID inID,
                                   AudioUnitScope inScope,
                                   AudioUnitElement inElement, void *outData) {
    if (inScope == kAudioUnitScope_Global) {
        if (inID == kAudioUnitProperty_MIDIOutputCallbackInfo) {
            CFStringRef strs[1];
            strs[0] = CFSTR("MIDI Callback");
            
            CFArrayRef callbackArray =
            CFArrayCreate(NULL, (const void **)strs, 1, &kCFTypeArrayCallBacks);
            *(CFArrayRef *)outData = callbackArray;
            return noErr;
        }
    }
    return AUMonotimbralInstrumentBase::GetProperty(inID, inScope, inElement,
                                                    outData);
}

OSStatus CC2Note::SetProperty(AudioUnitPropertyID inID,
                                   AudioUnitScope inScope,
                                   AudioUnitElement inElement,
                                   const void *inData, UInt32 inDataSize) {
#ifdef DEBUG
    DEBUGLOG_B("SetProperty" << endl);
#endif
    if (inScope == kAudioUnitScope_Global) {
        if (inID == kAudioUnitProperty_MIDIOutputCallback) {
            if (inDataSize < sizeof(AUMIDIOutputCallbackStruct))
                return kAudioUnitErr_InvalidPropertyValue;
            
            AUMIDIOutputCallbackStruct *callbackStruct =
            (AUMIDIOutputCallbackStruct *)inData;
            mCallbackHelper.SetCallbackInfo(callbackStruct->midiOutputCallback,
                                            callbackStruct->userData);
            return noErr;
        }
    }
    return AUMonotimbralInstrumentBase::SetProperty(inID, inScope, inElement,
                                                    inData, inDataSize);
}

OSStatus CC2Note::HandleMidiEvent(UInt8 status, UInt8 channel, UInt8 data1,
                                       UInt8 data2, UInt32 inStartFrame) {
    // data1 : note number, data2 : velocity
    
#ifdef DEBUG
    DEBUGLOG_B("HandleMidiEvent - status:"
               << (int)status << " ch:" << (int)channel << "/"
               << (Globals()->GetParameter(kParameter_Ch) - 1)
               << " data1:" << (int)data1 << " data2:" << (int)data2 << endl);
    DEBUGLOG_B("noteNumber = " << noteNumber << ", isNoteOff = " << isNoteOff << endl);
#endif
    
    int _ch = Globals()->GetParameter(kParameter_Ch);
    int _cc = Globals()->GetParameter(kParameter_CC);
    if (channel + 1 == _ch && status == kControlChange && data1 == _cc)
    {
        // ijiru
        if (isNoteOff && data2 > 0){
            isNoteOff = false;
            
            int min = Globals()->GetParameter(kParameter_NoteMin);
            int max = Globals()->GetParameter(kParameter_NoteMax);
            int velocity = Globals()->GetParameter(kParameter_Velocity);
                
            if(noteNumber > max || noteNumber < min)
                noteNumber = min;
                
            mCallbackHelper.AddMIDIEvent(kNoteOn, channel, noteNumber, velocity, inStartFrame);
            
        } else if(!isNoteOff && data2 == 0){
            mCallbackHelper.AddMIDIEvent(kNoteOff, channel, noteNumber, 0, inStartFrame);
            noteNumber++;
            isNoteOff = true;
        }
    } else
        mCallbackHelper.AddMIDIEvent(status, channel, data1, data2, inStartFrame);
    
    return AUMIDIBase::HandleMidiEvent(status, channel, data1, data2,
                                       inStartFrame);
}

OSStatus CC2Note::Render(AudioUnitRenderActionFlags &ioActionFlags,
                              const AudioTimeStamp &inTimeStamp,
                              UInt32 inNumberFrames) {
    
    OSStatus result =
    AUInstrumentBase::Render(ioActionFlags, inTimeStamp, inNumberFrames);
    if (result == noErr) {
        mCallbackHelper.FireAtTimeStamp(inTimeStamp);
    }
    return result;
}
