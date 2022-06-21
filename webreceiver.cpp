#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <cstdint>
#include <filesystem>
#include <sys/wait.h>
#include <sys/stat.h>
#include "global.h"
#include "webreceiver.h"
#include "server.h"

cWebReceiver::cWebReceiver(const cChannel *Channel, int Priority) : cReceiver(Channel, Priority) {
    // debug_plugin("Called with channel %s\n", Channel->Name());
    webReceiver = this;
    ffmpegHls = nullptr;

    audioStreamPID = cDevice::PrimaryDevice()->GetTrack(cDevice::PrimaryDevice()->GetCurrentAudioTrack())->id;
    copyVideo = Channel->Vtype() == 0x1B;
}

cWebReceiver::~cWebReceiver() {
    debug_plugin(" ");
    webReceiver = nullptr;

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
    }

    ffmpegHls = nullptr;
}

void cWebReceiver::Activate(bool On) {
    if (On) {
        ffmpegHls = new cFFmpegHLS(copyVideo, audioStreamPID);
    } else {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }

    cReceiver::Activate(On);
}

void cWebReceiver::Receive(const uchar *Data, int Length) {
    // debug_plugin("Data length %d, stream_fifo %d", Length, stream_fifo);
    if (ffmpegHls != nullptr) {
        ffmpegHls->Receive(Data, Length);
    }
}

cWebReceiver *webReceiver;
cDevice *webReceiverDevice;
