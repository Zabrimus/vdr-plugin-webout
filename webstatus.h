#ifndef WEBOUT_WEBSTATUS_H
#define WEBOUT_WEBSTATUS_H

#include <vdr/status.h>

class cWebStatus : public cStatus {
protected:
    void SetAudioTrack(int Index, const char * const *Tracks) override;
    // The audio track has been set to the one given by Index, which
    // points into the Tracks array of strings. Tracks is NULL terminated.

    void ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) override;
    // Indicates a channel switch on the given DVB device.
    // If ChannelNumber is 0, this is before the channel is being switched,
    // otherwise ChannelNumber is the number of the channel that has been switched to.
    // LiveView tells whether this channel switch is for live viewing.
};

#endif // WEBOUT_WEBSTATUS_H
