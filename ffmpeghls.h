#ifndef WEBOUT_FFMPEGHLS_H
#define WEBOUT_FFMPEGHLS_H

#include <filesystem>
#include <process.hpp>
#include <vdr/tools.h>

extern const char* STREAM_DIR;

class cFFmpegHLS {
private:
    TinyProcessLib::Process *ffmpegProcess;

public:
    cFFmpegHLS(bool copyVideo);
    ~cFFmpegHLS();

    void Receive(const uchar *Data, int Length);
};


#endif // WEBOUT_FFMPEGHLS_H
