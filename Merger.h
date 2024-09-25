#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <chrono>

#include "data/Capture.h"
#include "ErrorsConfig.h"
#include "sound_files/WaveFile.h"

#include <windows.h>

class Merger {
    public:
    std::queue<pair<BYTE *, UINT64>> dataQueue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> keepOn {true};

    WaveCreator *waveCreator = NULL;

    WAVEFORMATEX *format;
    bool processedFormat = false;

    public:
        ~Merger();

        int startRecording();
        int startStreaming();

        int start();
};
