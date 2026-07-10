#pragma once
#include <windows.h>

class IAudioRecorder {
public:
    virtual ~IAudioRecorder() = default;
    virtual WAVEFORMATEX* getFormat() = 0;
    virtual void collectSound(BYTE* destBuffer, UINT64& bytesReceived, UINT64 bufferLimit) = 0;
    virtual bool isEnough(UINT64 received, UINT64 bytesInSecond, long double chunkSeconds) = 0;
    virtual UINT64 getBytesInSecond() const = 0;
    virtual void startRecording() = 0;
};
