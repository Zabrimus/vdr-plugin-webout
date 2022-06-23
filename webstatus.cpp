#include "webstatus.h"
#include "webreceiver.h"
#include "server.h"

void cWebStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView) {
    printf("Channel Switch %d, Live %s\n", ChannelNumber, LiveView ? "ja" : "nein");

    if (!LiveView) {
        return;
    }

    printf("Channel Switch 2\n");

    if (ChannelNumber == 0) {
        cWebReceiver::deleteReceiver();
        // return;
    }

    printf("Channel Switch 3\n");

    printf("Send Switch to Browser\n");
    webOsdServer->sendPlayerReset();

    printf("Channel Switch 4\n");

    // recreate the receiver
    cWebReceiver::createReceiver();
}
