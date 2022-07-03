#include "global.h"
#include "webdevice.h"

const int maxTs = 256;
uint8_t tsBuffer[maxTs * 188];
int tsBufferIdx = 0;

cWebDevice *webDevice;

cWebDevice::cWebDevice() {
    debug_plugin(" ");
    webDevice = this;
}

cWebDevice::~cWebDevice() {
    debug_plugin(" ");
    webDevice = nullptr;

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
        fprintf(stderr, "===> LÄNGE: %d\n", Length);
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

    return Length;
}

bool cWebDevice::Flush(int TimeoutMs) {
    debug_plugin(" ");
    return true;
}

bool cWebDevice::Poll(cPoller &Poller, int TimeoutMs) {
    debug_plugin(" ");
    return true;
}

void cWebDevice::MakePrimaryDevice(bool On) {
    debug_plugin("ON: %s", On ? "true" : "false");
    cDevice::MakePrimaryDevice(On);

    if (On) {
        new cWebOsdProvider();
    }
}

void cWebDevice::Activate(bool On) {
    int replyCode = 0;
    cString result;

    debug_plugin("ON: %s", On ? "true" : "false");
    MakePrimaryDevice(On);

    for (int i = 0; i < NumDevices(); ++i) {
        debug_plugin("Device %d -> %s", i, *GetDevice(i)->DeviceName());
    }

    if (On) {
        // TODO: Copy Video bestimmen
        ffmpegHls = new cFFmpegHLS(false);

        // detach possible existing softhd* device
        result = sendSVDRPCommand("softhd", true, "DETA", "", replyCode);
        debug_plugin("Send DETA: %d -> %s", replyCode, *result);

        SetPrimaryDevice(DeviceNumber()+1);
    } else {
        if (ffmpegHls != nullptr) {
            delete ffmpegHls;
            ffmpegHls = nullptr;
        }

        // attach possible existing softhd* device
        result = sendSVDRPCommand("softhd", true, "ATTA", "", replyCode);
        debug_plugin("Send ATTA: %d -> %s", replyCode, *result);

        // TODO: DeviceNumber vom ursprünglichen PrimaryDevice sichern und hier nutzen
        SetPrimaryDevice(1);
    }
}

void cWebDevice::GetOsdSize(int &Width, int &Height, double &Aspect) {
    // cDevice::GetOsdSize(Width, Height, Aspect);
    Width = 1920;
    Height = 1080;
    Aspect = 16.0/9.0;
}
