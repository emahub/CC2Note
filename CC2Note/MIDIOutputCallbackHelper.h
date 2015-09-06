//
//  MIDIOutputCallbackHelper.h
//  ChCopy
//
//  Created by ema on 2014/04/29.
//
//

#ifndef __MIDIOutputCallbackHelper__
#define __MIDIOutputCallbackHelper__

#include <iostream>
#include <CoreMIDI/CoreMIDI.h>
#include <vector>

#endif /* defined(__MIDIOutputCallbackHelper__) */

typedef struct MIDIMessageInfoStruct {
  UInt8 status;
  UInt8 channel;
  UInt8 data1;
  UInt8 data2;
  UInt32 startFrame;
} MIDIMessageInfoStruct;

class MIDIOutputCallbackHelper {
  enum { kSizeofMIDIBuffer = 512 };

 public:
  MIDIOutputCallbackHelper() {
    mMIDIMessageList.reserve(64);
    mMIDICallbackStruct.midiOutputCallback = NULL;
    mMIDIBuffer = new Byte[kSizeofMIDIBuffer];
  }

  ~MIDIOutputCallbackHelper() { delete[] mMIDIBuffer; }

  void SetCallbackInfo(AUMIDIOutputCallback &callback, void *userData) {
    mMIDICallbackStruct.midiOutputCallback = callback;
    mMIDICallbackStruct.userData = userData;
  }

  void AddMIDIEvent(UInt8 status, UInt8 channel, UInt8 data1, UInt8 data2,
                    UInt32 inStartFrame);

  void FireAtTimeStamp(const AudioTimeStamp &inTimeStamp);

 private:
  MIDIPacketList *PacketList() { return (MIDIPacketList *)mMIDIBuffer; }

  Byte *mMIDIBuffer;

  AUMIDIOutputCallbackStruct mMIDICallbackStruct;

  typedef std::vector<MIDIMessageInfoStruct> MIDIMessageList;
  MIDIMessageList mMIDIMessageList;
};
