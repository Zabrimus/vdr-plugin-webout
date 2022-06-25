#include <iostream>
#include "global.h"
#include "webreceiver.h"
#include "server.h"

cWebReceiver *cWebReceiver::current;

cWebReceiver::cWebReceiver() {
    current = this;
    cReceiver::SetPriority(0);

    LOCK_CHANNELS_READ;
    int channel_nr = cDevice::CurrentChannel();
    const cChannel *channel = Channels->GetByNumber(channel_nr);

    SetPids(nullptr);

    currentAudioPid = getCurrentAudioPID();
    AddPid(currentAudioPid);

    printf("Current Audio PID %d\n", currentAudioPid);

    // AddPids(channel->Apids());
    AddPid(channel->Vpid());

    cDevice::ActualDevice()->AttachReceiver(this);
}

cWebReceiver::~cWebReceiver() {
    cReceiver::Detach();
    current = nullptr;

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }
}

void cWebReceiver::channelSwitch() {
    printf("ChanneSwitch\n");

    LOCK_CHANNELS_READ;
    int channel_nr = cDevice::CurrentChannel();
    const cChannel *channel = Channels->GetByNumber(channel_nr);

    SetPids(nullptr);
    AddPids(channel->Apids());
    AddPid(channel->Vpid());

    printf("Attached %d\n", NumPids());
}

int cWebReceiver::getCurrentAudioPID() {
    auto audioTrackType = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
    if (audioTrackType != ttNone) {
        auto audioTrack = cDevice::PrimaryDevice()->GetTrack(audioTrackType);
        return audioTrack->id;
    } else {
        return -1;
    }
}

void cWebReceiver::Activate(bool On) {
    printf("Activate: %s\n", On ? "true" : "false");

    if (On) {
        ffmpegHls = new cFFmpegHLS(copyVideo);;
    }
}

void cWebReceiver::Receive(const uchar *Data, int Length) {
    if (ffmpegHls != nullptr) {
        // printf("==> Receive: %d\n", Length);
        ffmpegHls->Receive(Data, Length);
    }
}
