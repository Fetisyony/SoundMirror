#include "presenter.h"
#include <windows.h>

Presenter::~Presenter() {
    delete waveCreator;
}

int Presenter::start() {
    int hr = OK;

    std::thread t1(&Presenter::startRecording, this);
    std::thread t2(&Presenter::startStreaming, this);

    Sleep(30000);
    done = true;

    t1.join();
    t2.join();

    waveCreator->closeFile();

    return hr;
}

int Presenter::startRecording() {
    int hr = OK;

    double secondsToCollect = 0.3;

    Capture *capture = new Capture();
    capture->startSoundCapture();

    capture->getFormat(&format);
    cout << "[recording] Received a format" << endl;

    cout << "[recording] Gonna notify streamer" << endl;
    cv.notify_one();
    cout << "[recording] Notified the streamer" << endl;

    UINT64 bytesInSecond = capture->getBytesInSecond();

    UINT64 bufferSize = bytesInSecond * secondsToCollect * 5;

    while (!done) {
        BYTE *buffer = new BYTE[bufferSize];
        UINT64 totalReceived = 0;
        capture->collectSound(secondsToCollect, buffer, &totalReceived, bufferSize);
        {
            std::lock_guard<std::mutex> lock(mtx);
            dataQueue.push({buffer, totalReceived});
            cout << "[recording] Pushed to queue" << endl;
        }
        cv.notify_one();
    }

    return hr;
}

int Presenter::startStreaming() {
    int hr = OK;

    while (!done) {
        std::unique_lock<std::mutex> lock(mtx);
        cout << "[streaming] Waiting..." << endl;
        cv.wait(lock, [this] { return !dataQueue.empty() || done; });
        cout << "[streaming] Finally we are here!" << endl;

        if (done) continue;
    
        if (!processedFormat) {
            waveCreator = new WaveCreator(this->format);
            processedFormat = true;
            cout << "[streaming] Processed a format - " << endl;
        }
        while (!dataQueue.empty()) {
            BYTE *data;
            UINT64 totalReceived;
            tie(data, totalReceived) = dataQueue.front();
            
            dataQueue.pop();
            lock.unlock();

            // kinda simulating
            waveCreator->addSound(data, totalReceived);
            std::cout << "[streaming] Received bytes number: " << totalReceived << std::endl;

            delete data;
            cout << "[streaming] Gonna lock..." << endl;
            lock.lock();
            cout << "[streaming] Locked..." << endl;
        }
    }

    return hr;
}
