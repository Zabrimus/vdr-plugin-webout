#include "global.h"
#include "webdevice.h"
#include "server.h"

const int maxTs = 256;
uint8_t tsBuffer[maxTs * 188];
int tsBufferIdx = 0;

cWebDevice *webDevice;

cWebDevice::cWebDevice() {
    debug_plugin(" ");
    webDevice = this;
    lastPrimaryDevice = -1;
}

cWebDevice::~cWebDevice() {
    debug_plugin(" ");
    webDevice = nullptr;

    if (webStatus != nullptr) {
        delete webStatus;
    }

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
        ffmpegHls = nullptr;
    }
}

bool cWebDevice::HasDecoder() const {
    // debug_plugin(" ");
    return true;
}

bool cWebDevice::SetPlayMode(ePlayMode PlayMode) {
    debug_plugin(" ");
    return true;
}

int cWebDevice::PlayVideo(const uchar *Data, int Length) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayAudio(const uchar *Data, int Length, uchar Id) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayTsVideo(const uchar *Data, int Length) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayTsAudio(const uchar *Data, int Length) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayTsSubtitle(const uchar *Data, int Length) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayPes(const uchar *Data, int Length, bool VideoOnly) {
    debug_plugin(" ");
    return Length;
}

int cWebDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly) {
    // debug_plugin("Length: %d", Length);

    if (Length > 188) {
        // fprintf(stderr, "===> LÄNGE: %d\n", Length);
    }

    if (ffmpegHls != nullptr) {
        if (Length == 188) {
            if (tsBufferIdx < maxTs - 1) {
                memcpy(tsBuffer + tsBufferIdx * 188, Data, Length);
                tsBufferIdx++;
            } else {
                memcpy(tsBuffer + tsBufferIdx * 188, Data, Length);
                ffmpegHls->Receive((const uint8_t *) tsBuffer, maxTs * 188);
                tsBufferIdx = 0;
            }
        } else {
            // recording
            ffmpegHls->Receive((const uint8_t *) Data, Length);
            tsBufferIdx = 0;
        }
    }

    return Length;
}

bool cWebDevice::Flush(int TimeoutMs) {
    debug_plugin(" ");
    return true;
}

bool cWebDevice::Poll(cPoller &Poller, int TimeoutMs) {
    // debug_plugin(" ");
    return true;
}

void cWebDevice::MakePrimaryDevice(bool On) {
    if (On) {
        new cWebOsdProvider();
    }
}

void cWebDevice::Activate(bool On) {
    if (On) {
        lastPrimaryDevice = cDevice::PrimaryDevice()->DeviceNumber();

        // TODO: Copy Video bestimmen
        ffmpegHls = new cFFmpegHLS(false);

        // switch primary device to webdevice
        Setup.PrimaryDVB = DeviceNumber() + 1;

        webStatus = new cWebStatus();
    } else {
        if (ffmpegHls != nullptr) {
            delete ffmpegHls;
            ffmpegHls = nullptr;
        }

        if (webStatus != nullptr) {
            DELETENULL(webStatus);
        }

        // switch primary device back to the former one
        if (lastPrimaryDevice != -1) {
            Setup.PrimaryDVB = lastPrimaryDevice + 1;
            lastPrimaryDevice = -1;
        }
    }
}

void cWebDevice::GetOsdSize(int &Width, int &Height, double &Aspect) {
    Width = 1920;
    Height = 1080;
    Aspect = 16.0/9.0;
}

void cWebDevice::channelSwitch() {
    printf("ChannelSwitch\n");

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
    }

    // TODO: Copy video
    ffmpegHls = new cFFmpegHLS(false);

    webOsdServer->sendPlayerReset();
}

void cWebDevice::changeAudioTrack() {
    // printf("ChangeAudioTrack from %d to %d\n", currentAudioPid, getCurrentAudioPID());

    // TODO: currently disabled
    return;

    // DelPid(currentAudioPid);
    // AddPid(getCurrentAudioPID());

    // TODO: Der Videoplayer müsste wahrscheinlich über den Wechsel benachrichtigt werden.
}

void cWebDevice::Replaying(bool On) {
    printf("Replaying: %s\n", On ? "An" : "Aus");

    if (ffmpegHls != nullptr) {
        delete ffmpegHls;
    }

    // TODO: Copy video
    ffmpegHls = new cFFmpegHLS(false);

    webOsdServer->sendPlayerReset();
}
