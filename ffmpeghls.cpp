#include <iomanip>
#include <vdr/device.h>
#include "global.h"
#include "ffmpeghls.h"

const char* STREAM_DIR = "/tmp/vdr-live-tv-stream";

const std::string VIDEO_ENCODE_H264 = "-crf 23 -c:v libx264 -tune zerolatency -vf format=yuv420p -preset ultrafast -qp 0 ";
const std::string VIDEO_ENCODE_COPY = "-c:v copy";
const std::string AUDIO_ENCODE_AAC = "-c:a aac -b:a 192k";
const std::string AUDIO_ENCODE_COPY = "-c:a copy";

cFFmpegHLS::cFFmpegHLS(bool copyVideo) {
    std::filesystem::remove_all(STREAM_DIR);
    std::filesystem::create_directories(STREAM_DIR);

    // std::string ffmpeg = std::string("ffmpeg -i - -v panic -hide_banner -ignore_unknown -fflags flush_packets -max_delay 5 -flags -global_header -hls_time 5 -hls_list_size 3 ") +
    std::string ffmpeg = std::string("ffmpeg -i - -ignore_unknown -fflags flush_packets -max_delay 5 -flags -global_header -hls_time 5 -hls_list_size 3 ") +
            std::string(" -map 0:v -map 0:a?") +
            std::string(" ") + (copyVideo ? VIDEO_ENCODE_COPY : VIDEO_ENCODE_H264) +
            std::string(" ") + (IS_DOLBY_TRACK(cDevice::PrimaryDevice()->GetCurrentAudioTrack()) ? AUDIO_ENCODE_COPY : AUDIO_ENCODE_AAC) +
            std::string(" -y vdr-live-tv.m3u8");

    printf("=> Call ffmpeg with '%s'\n", ffmpeg.c_str());

    ffmpegProcess = new TinyProcessLib::Process(ffmpeg, STREAM_DIR, nullptr, nullptr, true);

    int exitStatus;

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        printf("ffmpeg error_code: %d\n", exitStatus);
    }
}

cFFmpegHLS::~cFFmpegHLS() {
    if (ffmpegProcess != nullptr) {
        ffmpegProcess->close_stdin();
        ffmpegProcess->kill(true);
        ffmpegProcess->get_exit_status();

        ffmpegProcess = nullptr;
    }

    /*
    std::error_code ec;
    std::filesystem::remove_all(STREAM_DIR, ec);

    if (ec.value() != 0) {
        printf("Error %s\n", ec.message().c_str());
    }
    */

    system((std::string("rm -rf ") + STREAM_DIR + "/*.ts").c_str());
    system((std::string("rm -rf ") + STREAM_DIR + "/*.m3u8").c_str());
}

void cFFmpegHLS::Receive(const uchar *Data, int Length) {
    int exitStatus;

    if (ffmpegProcess->try_get_exit_status(exitStatus)) {
        // process stopped/finished/crashed
        printf("2=> ffmpeg error_code: %d\n", exitStatus);
        return;
    }

    if (!ffmpegProcess->write((const char*)Data, Length)) {
        debug_plugin("Cannot write %d", Length);
    }
}
