#include <iostream>
#include "global.h"
#include "webreceiver.h"
#include "webplayer.h"
#include "server.h"


const int maxTs = 256;
uint8_t tsBuffer[maxTs * 188];
int tsBufferIdx = 0;

cWebReceiver *cWebReceiver::current;

cWebReceiver::cWebReceiver() {
    current = this;
    cReceiver::SetPriority(0);

    LOCK_CHANNELS_READ;
    int channel_nr = cDevice::CurrentChannel();
    const cChannel *channel = Channels->GetByNumber(channel_nr);

    SetPids(nullptr);

    // currentAudioPid = getCurrentAudioPID();
    // AddPid(currentAudioPid);

    AddPids(channel->Apids());
    AddPid(channel->Vpid());

    cDevice::ActualDevice()->AttachReceiver(this);

    ffmpegHls = new cFFmpegHLS(copyVideo);
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
    printf("ChannelSwitch\n");

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
    }

    LOCK_CHANNELS_READ;
    int channel_nr = cDevice::CurrentChannel();
    const cChannel *channel = Channels->GetByNumber(channel_nr);

    SetPids(nullptr);
    AddPids(channel->Apids());
    // AddPid(getCurrentAudioPID());
    AddPid(channel->Vpid());

    ffmpegHls = new cFFmpegHLS(copyVideo);

    cDevice::ActualDevice()->AttachReceiver(this);

    webOsdServer->sendPlayerReset();

    auto ctrl = new cWebControl(new cWebPlayer);
    cControl::Launch(ctrl);
    cControl::Attach();

    printf("Attached %d\n", NumPids());
}

void cWebReceiver::changeAudioTrack() {
    printf("ChangeAudioTrack from %d to %d\n", currentAudioPid, getCurrentAudioPID());

    // TODO: currently disabled
    return;

    // DelPid(currentAudioPid);
    // AddPid(getCurrentAudioPID());

    // TODO: Der Videoplayer müsste wahrscheinlich über den Wechsel benachrichtigt werden.
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
        // ffmpegHls = new cFFmpegHLS(copyVideo);
    }
}

void cWebReceiver::Receive(const uchar *Data, int Length) {
    if (Length != 188) {
        printf("ERROR: Length %d\n", Length);
    }

    if (ffmpegHls != nullptr) {
        if (tsBufferIdx < maxTs - 1) {
            memcpy(tsBuffer + tsBufferIdx*188, Data, Length);
            tsBufferIdx++;
        } else {
            // printf("Receive: %d -> %d\n", tsBufferIdx, tsBufferIdx * 188);
            memcpy(tsBuffer + tsBufferIdx*188, Data, Length);
            ffmpegHls->Receive((const uint8_t*)tsBuffer, maxTs * 188);
            tsBufferIdx = 0;
        }

        // printf("==> Receive: %d\n", Length);
        // ffmpegHls->Receive(Data, Length);
    }
}
