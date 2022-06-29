#include "global.h"
#include "webplayer.h"

cWebPlayer::cWebPlayer() {
    debug_plugin(" ");
}

cWebPlayer::~cWebPlayer() {
    debug_plugin(" ");
}

void cWebPlayer::Activate(bool On) {
    debug_plugin(" %s", On ? "On" : "Off");
    cPlayer::Activate(On);
}

cWebControl::cWebControl(cWebPlayer* player) :cControl(player) {
    debug_plugin(" ");
}

cWebControl::~cWebControl() {
    debug_plugin(" ");

    if (player) {
        delete player;
    }
}

void cWebControl::Hide() {
    debug_plugin(" ");
}

cOsdObject *cWebControl::GetInfo() {
    debug_plugin(" ");
    return cControl::GetInfo();
}

eOSState cWebControl::ProcessKey(eKeys Key) {
    // debug_plugin(" ");
    return cOsdObject::ProcessKey(Key);
}
