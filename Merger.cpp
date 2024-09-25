#include "Merger.h"

#define IS_NOT_OK(rc) (rc != OK)

const char *waveOutputFilename = "output.wav";
const int secondsToWork = 20;

const double chunkInSecondsToCollect = 0.3;

Merger::~Merger() {
    delete waveCreator;
}

int Merger::start() {
    int rc = OK;

    std::thread t1(&Merger::startRecording, this);
    std::thread t2(&Merger::startStreaming, this);

    Sleep(secondsToWork * 1000);
    keepOn.store(false);

    t1.join();
    t2.join();

    (void)waveCreator->closeFile();

    return rc;
}

int Merger::startRecording() {
    int rc = OK;

    UINT64 bytesInSecond;
    UINT64 bufferSize;

    Capture *capture = NULL;
    try {
        capture = new Capture();
    } catch (const std::runtime_error& e) {
        std::cerr << "Capture constructor error: " << e.what() << std::endl;
        goto done;
    }
    rc = capture->startSoundCaptureWrapper();
    if (IS_NOT_OK(rc)) goto done;

    capture->getFormat(&format);
    cout << "[recording] Received a format" << endl;

    cout << "[recording] Gonna notify streamer" << endl;
    cv.notify_one();
    cout << "[recording] Notified the streamer" << endl;

    bytesInSecond = capture->getBytesInSecond();
    bufferSize = bytesInSecond * chunkInSecondsToCollect * 2;

    while (keepOn.load()) {
        BYTE *buffer = new BYTE[bufferSize];
        UINT64 totalReceived = 0;
        rc = capture->collectSound(chunkInSecondsToCollect, buffer, &totalReceived, bufferSize);
        if (IS_NOT_OK(rc)) goto done;

        {
            std::lock_guard<std::mutex> lock(mtx);
            dataQueue.push({buffer, totalReceived});
            cout << "[recording] Pushed to queue" << endl;
        }
        cv.notify_one();
    }

done:
    if (capture)
        delete capture;
    keepOn.store(false);
    return rc;
}

int Merger::startStreaming() {
    int rc = OK;

    while (keepOn.load()) {
        std::unique_lock<std::mutex> lock(mtx);
        cout << "[streaming] Waiting..." << endl;
        cv.wait(lock, [this] { return !dataQueue.empty() || !keepOn.load(); });
        cout << "[streaming] Finally we are here!" << endl;

        if (!keepOn.load()) continue;

        if (!processedFormat) {
            try {
                waveCreator = new WaveCreator(waveOutputFilename, this->format);
            } catch (const std::runtime_error& e) {
                std::cerr << "WaveCreator constructor error: " << e.what() << std::endl;
                keepOn.store(false);
            }
            processedFormat = true;
            cout << "[streaming] Processed a format - " << endl;
        }
        while (!dataQueue.empty()) {
            BYTE *data; UINT64 totalReceived;
            tie(data, totalReceived) = dataQueue.front();
            dataQueue.pop();

            lock.unlock();

            // kinda simulating
            rc = waveCreator->addSound(data, totalReceived);
            if (IS_NOT_OK(rc)) goto done;
            std::cout << "[streaming] Received bytes number: " << totalReceived << std::endl;

            delete data;

            cout << "[streaming] Gonna lock..." << endl;
            lock.lock();
            cout << "[streaming] Locked..." << endl;
        }
    }

done:
    return rc;
}
