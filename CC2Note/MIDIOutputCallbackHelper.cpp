//
//  MIDIOutputCallbackHelper.cpp
//

#include "MIDIOutputCallbackHelper.h"

void MIDIOutputCallbackHelper::AddMIDIEvent(UInt8 status, UInt8 channel,
                                            UInt8 data1, UInt8 data2,
                                            UInt32 inStartFrame) {
  MIDIMessageInfoStruct info = {status, channel, data1, data2, inStartFrame};
  mMIDIMessageList.push_back(info);
}

void MIDIOutputCallbackHelper::FireAtTimeStamp(
    const AudioTimeStamp &inTimeStamp) {
  if (!mMIDIMessageList.empty()) {
    if (mMIDICallbackStruct.midiOutputCallback) {
      // synthesize the packet list and call the MIDIOutputCallback
      // iterate through the vector and get each item
      MIDIPacketList *pktlist = PacketList();

      MIDIPacket *pkt = MIDIPacketListInit(pktlist);

      for (MIDIMessageList::iterator iter = mMIDIMessageList.begin();
           iter != mMIDIMessageList.end(); iter++) {
        const MIDIMessageInfoStruct &item = *iter;

        Byte midiStatusByte = item.status + item.channel;
        const Byte data[3] = {midiStatusByte, item.data1, item.data2};
        UInt32 midiDataCount =
            ((item.status == 0xC || item.status == 0xD) ? 2 : 3);

        pkt = MIDIPacketListAdd(pktlist, kSizeofMIDIBuffer, pkt,
                                item.startFrame, midiDataCount, data);
        if (!pkt) {
          // send what we have and then clear the buffer and then go through
          // this again issue the callback with what we got
          OSStatus result = (*mMIDICallbackStruct.midiOutputCallback)(
              mMIDICallbackStruct.userData, &inTimeStamp, 0, pktlist);
          if (result != noErr)
            printf("error calling output callback: %d", (int)result);

          // clear stuff we've already processed, and fire again
          mMIDIMessageList.erase(mMIDIMessageList.begin(), iter);
          FireAtTimeStamp(inTimeStamp);
          return;
        }
      }

      // fire callback
      OSStatus result = (*mMIDICallbackStruct.midiOutputCallback)(
          mMIDICallbackStruct.userData, &inTimeStamp, 0, pktlist);
      if (result != noErr)
        printf("error calling output callback: %d", (int)result);
    }
    mMIDIMessageList.clear();
  }
}
