#ifndef WEBOUT_WEBPLAYER_H
#define WEBOUT_WEBPLAYER_H

#include <vdr/player.h>

class cWebPlayer : public cPlayer {
protected:
    virtual void Activate(bool On);

public:
    cWebPlayer();
    ~cWebPlayer() override;
};

class cWebControl : public cControl {
public:
    cWebControl(cWebPlayer *player);
    ~cWebControl() override;
    void Hide() override;
    cOsdObject *GetInfo() override;
    eOSState ProcessKey(eKeys Key) override;
};

extern cWebPlayer *webPlayer;

#endif // WEBOUT_WEBPLAYER_H
