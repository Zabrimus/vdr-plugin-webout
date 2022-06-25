#include "webstatus.h"
#include "webreceiver.h"
#include "server.h"

void cWebStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) {
    printf("Channel Switch %d, Live %s\n", ChannelNumber, LiveView ? "ja" : "nein");

    if (!LiveView || ChannelNumber == 0) {
        return;
    }

    // recreate the receiver
    cWebReceiver::getCurrentWebReceiver()->channelSwitch();
}

void cWebStatus::SetAudioTrack(int Index, const char *const *Tracks) {
    printf("Set Audio Track to %d\n", Index);
    printf("Track: %s\n", Tracks[Index]);

    cWebReceiver::getCurrentWebReceiver()->changeAudioTrack();
}
