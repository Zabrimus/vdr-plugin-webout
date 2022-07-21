#ifndef WEBOUT_WEBENCODER_H
#define WEBOUT_WEBENCODER_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>

extern "C" {
#include <libavformat/avformat.h>
}

#include "ring_buffer.hpp"

class cWebEncoder {
private:
    static int bytes_remuxed;
    static std::mutex buf_mutex;
    static std::condition_variable cond;
    static std::atomic<bool> receiving_finished;
    static std::atomic<bool> remuxing_thread_done;

    static RingBuffer* buff;
    std::thread remuxing_thread;

    static bool make_input_ctx(AVFormatContext** input_ctx, AVIOContext** avio_input_ctx, RingBuffer* buff);
    static bool make_streams_map(AVFormatContext** input_ctx, int** streams_map);
    static int read_callback(void* opaque, uint8_t* buf, int buf_size);
    static bool transcode(AVFormatContext** input_ctx, AVFormatContext** output_ctx, int* streams_map);

public:
    cWebEncoder();
    ~cWebEncoder();

    int PlayTS(const uint8_t *Data, int Length);

    static void remux_worker(const std::string out_filename);
};

#endif // WEBOUT_WEBENCODER_H
