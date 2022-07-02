#ifndef WEBOUT_WEBDEVICE_H
#define WEBOUT_WEBDEVICE_H

#include <vdr/device.h>

class cWebDevice : public cDevice {
public:
    cWebDevice();
    ~cWebDevice() override;

    cString DeviceName() const override { return "Hallo"; };

    bool HasDecoder() const override;

    bool SetPlayMode(ePlayMode PlayMode) override;
    int PlayVideo(const uchar *Data, int Length) override;

    int PlayAudio(const uchar *Data, int Length, uchar Id) override;

    int PlayTsVideo(const uchar *Data, int Length) override;
    int PlayTsAudio(const uchar *Data, int Length) override;
    int PlayTsSubtitle(const uchar *Data, int Length) override;

    int PlayPes(const uchar *Data, int Length, bool VideoOnly) override;
    int PlayTs(const uchar *Data, int Length, bool VideoOnly) override;

    bool Poll(cPoller &Poller, int TimeoutMs) override;
    bool Flush(int TimeoutMs) override;

    void Activate(bool On);

protected:
    void MakePrimaryDevice(bool On) override;
};

extern cWebDevice *webDevice;

#endif // WEBOUT_WEBDEVICE_H
