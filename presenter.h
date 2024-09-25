#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>

#include "data/Capture.h"
#include "ErrorsConfig.h"
#include "sound_files/WaveFile.h"

class Presenter {
    public:
    std::queue<pair<BYTE *, UINT64>> dataQueue;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;

    WaveCreator *waveCreator = NULL;

    WAVEFORMATEX *format;
    bool processedFormat = false;

    bool dataReady = false;
    bool stopThreads = false;

    public:
        ~Presenter();

        int startRecording();
        int startStreaming();

        int start();
};
