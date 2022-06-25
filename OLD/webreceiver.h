#ifndef WEBOUT_WEBRECEIVER_H
#define WEBOUT_WEBRECEIVER_H

#include <process.hpp>
#include <vdr/receiver.h>
#include "ffmpeghls.h"

class cWebReceiver : public cReceiver {
private:
    int audioStreamPID;
    bool copyVideo;
    cFFmpegHLS *ffmpegHls;

    cWebReceiver(const cChannel *Channel = nullptr, int Priority = MINPRIORITY);
    ///< Creates a new receiver for the given Channel with the given Priority.
    ///< If Channel is not NULL, its pids are set by a call to SetPids().
    ///< Otherwise pids can be added to the receiver by separate calls to the AddPid[s]
    ///< functions.
    ///< The total number of PIDs added to a receiver must not exceed MAXRECEIVEPIDS.
    ///< Priority may be any value in the range MINPRIORITY...MAXPRIORITY. Negative values indicate
    ///< that this cReceiver may be detached at any time in favor of a timer recording
    ///< or live viewing (without blocking the cDevice it is attached to).

protected:
    void Activate(bool On);
    ///< This function is called just before the cReceiver gets attached to
    ///< (On == true) and right after it gets detached from (On == false) a cDevice. It can be used
    ///< to do things like starting/stopping a thread.
    ///< It is guaranteed that Receive() will not be called before Activate(true).

    void Receive(const uchar *Data, int Length);
    ///< This function is called from the cDevice we are attached to, and
    ///< delivers one TS packet from the set of PIDs the cReceiver has requested.
    ///< The data packet must be accepted immediately, and the call must return
    ///< as soon as possible, without any unnecessary delay. Each TS packet
    ///< will be delivered only ONCE, so the cReceiver must make sure that
    ///< it will be able to buffer the data if necessary.

public:
    static void createReceiver();
    static void deleteReceiver();

    ~cWebReceiver() override;
};

#endif // WEBOUT_WEBRECEIVER_H
