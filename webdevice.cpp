#include "global.h"
#include "webdevice.h"

cWebDevice *webDevice;

cWebDevice::cWebDevice() {
    debug_plugin(" ");
    webDevice = this;
}

cWebDevice::~cWebDevice() {
    debug_plugin(" ");
    webDevice = nullptr;
}

bool cWebDevice::HasDecoder() const {
    debug_plugin(" ");
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
    debug_plugin(" ");
    cDevice::MakePrimaryDevice(On);
}

void cWebDevice::Activate(bool On) {
    debug_plugin("ON: %s", On ? "true" : "false");
    MakePrimaryDevice(On);

    for (int i = 0; i < NumDevices(); ++i) {
        debug_plugin("Device %d -> %s", i, *GetDevice(i)->DeviceName());
    }

    if (On) {
        SetPrimaryDevice(DeviceNumber()+1);
    } else {
        SetPrimaryDevice(1);
    }

    debug_plugin("PrimaryDevice: %s", *PrimaryDevice()->DeviceName());
    debug_plugin("ActualDevice: %s", *ActualDevice()->DeviceName());
    debug_plugin("CardIndex: %d", CardIndex());
    debug_plugin("DeviceNumber: %d", DeviceNumber());
}
