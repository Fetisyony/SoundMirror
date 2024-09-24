#include "presenter.h"

int Presenter::start() {
    int hr = OK;

    std::thread t1(&Presenter::startRecording, this);
    std::thread t2(&Presenter::startStreaming, this);

    t1.join();
    t2.join();

    return hr;
}

int Presenter::startRecording() {
    int hr = OK;

    double secondsToCollect = 0.5;

    Capture *capture = new Capture();
    capture->startSoundCapture();

    capture->getFormat(&format);

    cv.notify_one();

    while (!done) {
        BYTE *buffer = new BYTE[1000000];
        capture->collectSound(secondsToCollect, buffer);
        {
            std::lock_guard<std::mutex> lock(mtx);
            dataQueue.push(buffer);
        }
        cv.notify_one();
    }

    return hr;
}

int Presenter::startStreaming() {
    int hr = OK;

    while (!done) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !dataQueue.empty() || done; });
        if (processedFormat) {
            processedFormat = true;
        }
        while (!dataQueue.empty()) {
            BYTE *data = dataQueue.front();
            dataQueue.pop();
            lock.unlock();

            // Simulate sending data over the network
            std::cout << "Sending data: " << data << std::endl;

            free(data);
            lock.lock();
        }
    }

    return hr;
}
