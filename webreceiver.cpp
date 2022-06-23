#include <iostream>
#include "global.h"
#include "webreceiver.h"
#include "server.h"

cWebReceiver *webReceiver;
cDevice *webReceiverDevice;

cWebReceiver::cWebReceiver(const cChannel *Channel, int Priority) : cReceiver(Channel, Priority) {
    // debug_plugin("Called with channel %s\n", Channel->Name());
    webReceiver = this;

    printf("Construct WebReceiver\n");

    auto audioTrackType = cDevice::PrimaryDevice()->GetCurrentAudioTrack();
    if (audioTrackType != ttNone) {
        auto audioTrack = cDevice::PrimaryDevice()->GetTrack(audioTrackType);
        audioStreamPID = audioTrack->id;
        copyVideo = Channel->Vtype() == 0x1B;

        printf("Starte ffmpeg\ņ");

        ffmpegHls = new cFFmpegHLS(copyVideo, audioStreamPID);;
    } else {
        printf("Kein Audiostream\ņ");
        
        audioStreamPID = 0;
        copyVideo = false;
    }
}

cWebReceiver::~cWebReceiver() {
    debug_plugin(" ");
    webReceiver = nullptr;
    webReceiverDevice = nullptr;

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }
}

void cWebReceiver::Activate(bool On) {
    printf("==> IN ACTIVATE : %s\n", On ? "An" : "Aus");

    /*
    if (On) {
        ffmpegHls = new cFFmpegHLS(copyVideo, audioStreamPID);
    } else {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }
    */
}

void cWebReceiver::Receive(const uchar *Data, int Length) {
    // debug_plugin("Data length %d, stream_fifo %d", Length, stream_fifo);
    if (ffmpegHls != nullptr) {

        printf("==> Receive: %d\n", Length);
        ffmpegHls->Receive(Data, Length);
    }
}

void cWebReceiver::createReceiver() {
    if (webReceiver != nullptr) {
        webReceiverDevice->Detach(webReceiver);
        delete webReceiver;
    }

    int channel_nr = cDevice::CurrentChannel();

    printf("=> ChannelNumber: %d\n", channel_nr );

    LOCK_CHANNELS_READ;
    const cChannel *channel = Channels->GetByNumber(channel_nr);

    printf("=> Channel %p\n", channel);

    if (channel != nullptr) {
        webReceiver = new cWebReceiver(channel);
        webReceiverDevice = cDevice::GetDevice(channel, 0, true, false);
        webReceiverDevice->AttachReceiver(webReceiver);
    }
}

void cWebReceiver::deleteReceiver() {
    if (webReceiver == nullptr && webReceiverDevice == nullptr) {
        return;
    }

    webReceiverDevice->Detach(webReceiver);
    delete webReceiver;
    webReceiver = nullptr;
    webReceiverDevice = nullptr;
}
