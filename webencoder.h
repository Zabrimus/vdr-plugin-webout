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
    std::thread remuxing_thread;

public:
    cWebEncoder();
    ~cWebEncoder();

    int PlayTS(const uint8_t *Data, int Length);
};

#endif // WEBOUT_WEBENCODER_H
