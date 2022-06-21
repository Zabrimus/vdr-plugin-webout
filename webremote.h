#ifndef WEBOUT_WEBREMOTE_H
#define WEBOUT_WEBREMOTE_H

#include <vdr/remote.h>

class cWebRemote : cRemote {
public:
    explicit cWebRemote();
    ~cWebRemote() override;

    bool ProcessKey(const char *Code, bool Repeat = false, bool Release = false);
};

extern cWebRemote webRemote;

#endif // WEBOUT_WEBREMOTE_H
