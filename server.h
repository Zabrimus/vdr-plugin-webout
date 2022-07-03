#ifndef WEBOUT_SERVER_H
#define WEBOUT_SERVER_H

#include <vdr/thread.h>
#include <App.h>
#include <WebSocket.h>
#include "webosd.h"
#include "webremote.h"

struct PerSocketData {
    /* Define your user data */
    int something;
};

class cWebOsdServer : public cThread {
private:
    int port = 3000;

    uWS::App *globalApp;
    uWS::WebSocket<false, true, PerSocketData> *gws;
    uWS::Loop *globalAppLoop;
    us_listen_socket_t *listenSocket;

    // cWebOsdProvider *osdProvider;
    // cWebStatus *webStatus;
    // cWebReceiver *webReceiver;

protected:
    void Action() override;
    ///< A derived cThread class must implement the code it wants to
    ///< execute as a separate thread in this function. If this is
    ///< a loop, it must check Running() repeatedly to see whether
    ///< it's time to stop.

public:
    explicit cWebOsdServer(const char *Description = nullptr, bool LowPriority = false);
    ///< Creates a new thread.
    ///< If Description is present, a log file entry will be made when
    ///< the thread starts and stops (see SetDescription()).
    ///< The Start() function must be called to actually start the thread.
    ///< LowPriority can be set to true to make this thread run at a lower
    ///< priority.

    void Cancel(int WaitSeconds = 0);
    ///< Cancels the thread by first setting 'running' to false, so that
    ///< the Action() loop can finish in an orderly fashion and then waiting
    ///< up to WaitSeconds seconds for the thread to actually end. If the
    ///< thread doesn't end by itself, it is killed.
    ///< If WaitSeconds is -1, only 'running' is set to false and Cancel()
    ///< returns immediately, without killing the thread.

    virtual ~cWebOsdServer();

    int sendPngImage(int x, int y, int w, int h, int bufferSize, uint8_t *buffer);
    int sendSize();
    int sendPlayerReset();
    int sendClearOsd();

    void receiveKeyEvent(std::string_view event);
};

extern cWebOsdServer *webOsdServer;

#endif // WEBOUT_SERVER_H
