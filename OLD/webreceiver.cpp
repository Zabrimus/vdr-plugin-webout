#include <signal.h>
#include <sys/wait.h>
#include "global.h"
#include "webreceiver.h"
#include "server.h"

void ffmpeg_killed(int sig) {
    fprintf(stderr, "Parent Signal\n");
    webReceiver->child_killed();
}

cWebReceiver::cWebReceiver(const cChannel *Channel, int Priority) : cReceiver(Channel, Priority) {
    // debug_plugin("Called with channel %s\n", Channel->Name());
    webReceiver = this;
    ffmpeg_running = false;
    stream_fifo = -1;
}

cWebReceiver::~cWebReceiver() {
    debug_plugin(" ");
    webReceiver = nullptr;
    stream_fifo = -1;
    ffmpeg_running = false;
}

void cWebReceiver::Activate(bool On) {
    if (On) {
        // fork and start ffmpeg
        ffmpeg_pid = fork();
        if (ffmpeg_pid == -1) {
            perror("fork");
        } else if (ffmpeg_pid > 0) {
            ffmpeg_running = true;
            signal(SIGCHLD, &ffmpeg_killed);
            sleep(1);
        } else {
            cString Command("ffmpeg -i /tmp/vdrstream -c:v copy -c:a copy x264.mp4");
            if (execl("/bin/sh", "sh", "-c", *Command, NULL) == -1) {
                fprintf(stderr, "Starting ffmpeg failed.");
                exit(-1);
            }
        }

        if (ffmpeg_running) {
            stream_fifo = open("/tmp/vdrstream", O_WRONLY);
        }
    } else {
        // close named pipe
        close(stream_fifo);

        // kill ffmpg if the process still exists
    }

    cReceiver::Activate(On);
}

void cWebReceiver::Receive(const uchar *Data, int Length) {
    // debug_plugin("Data length %d, stream_fifo %d", Length, stream_fifo);

    if (stream_fifo == -1) {
        return;
    }

    if (write(stream_fifo, Data, Length) == -1) {
        debug_plugin("Cannot write %d", Length);
        // could happen if ffmpeg is not yet ready or not started
        // this error will be ignored
    }

}

void cWebReceiver::child_killed() {
    ffmpeg_running = false;

    if (stream_fifo != -1) {
        close(stream_fifo);
    }

    stream_fifo = -1;
}

cWebReceiver *webReceiver;
cDevice *webReceiverDevice;
