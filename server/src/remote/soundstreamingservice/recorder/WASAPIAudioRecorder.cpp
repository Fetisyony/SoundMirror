#include "WASAPIAudioRecorder.hpp"

#include <functional>

#include "ErrorHandling.hpp"
#include "ErrorCodes.hpp"
#include "exceptions/ChunkRecordingFailure.hpp"
#include "exceptions/StartRecordingFailure.hpp"


bool WASAPIAudioRecorder::isEnough(const UINT64 received, const UINT64 bytesInSecond, const long double chunkSeconds) {
    return received >= bytesInSecond * chunkSeconds;
}

float getPeakVolume(BYTE* buffer, UINT32 frames, UINT32 blockAlign) {
    const float* floatBuffer = reinterpret_cast<float*>(buffer);
    const UINT32 floatCount = (frames * blockAlign) / sizeof(float);

    float maxVal = 0.0f;
    for (UINT32 i = 0; i < floatCount; i++) {
        float sample = std::abs(floatBuffer[i]);
        if (sample > maxVal) {
            maxVal = sample;
        }
    }
    return maxVal;
}

UINT64 WASAPIAudioRecorder::getBytesInSecond() const {
    return format->nSamplesPerSec * (format->wBitsPerSample / 8) * format->nChannels;
}

void WASAPIAudioRecorder::startRecording() {
    HRESULT hr = pAudioClient->Start();

    if (FAILED(hr))
        throw StartRecordingFailure(__FILE__, __FUNCTION__, __LINE__, ("Failed to start recording: " + std::to_string(hr)).c_str());
}

void WASAPIAudioRecorder::collectSound(BYTE *destBuffer, UINT64 &bytesReceived, UINT64 bufferLimit) {
    HRESULT hr = OK;

    constexpr float SILENCE_THRESHOLD = 1e-4f;

    bytesReceived = 0;

    UINT64 framesProcessed = 0;
    const UINT64 frameLimit = bufferLimit / format->nBlockAlign;

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    HANDLE_RET_CODE(hr, "GetNextPacketSize", done);

    while (framesProcessed < frameLimit && packetLength != 0) {
        if (packetLength > frameLimit) {
            spdlog::warn("One packet is larger than the entire buffer: {} vs {} (in frames units)", packetLength, frameLimit);
        }
        if (framesProcessed + packetLength > frameLimit)
            break;

        hr = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, nullptr, nullptr);
        HANDLE_RET_CODE(hr, "GetBuffer", done);

        if (nFrames == 0) { // not supposed to happen
            hr = pCaptureClient->ReleaseBuffer(nFrames);
            break;
        }

        float peakVol = getPeakVolume(captureBuffer, nFrames, format->nBlockAlign);
        bool isSilent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) || (peakVol < SILENCE_THRESHOLD);

        UINT64 bytesToCopy = nFrames * format->nBlockAlign;
        if (!isSilent) {
            // in shared mode, flags & SILENT == 1 means “zero data.”
            memcpy(destBuffer + bytesReceived, captureBuffer, bytesToCopy);
            bytesReceived += bytesToCopy;
        }
        framesProcessed += nFrames;

        // well, we took frames that we can accept, let's just skip ones that left
        hr = pCaptureClient->ReleaseBuffer(nFrames);
        HANDLE_RET_CODE(hr, "ReleaseBuffer", done);

        hr = pCaptureClient->GetNextPacketSize(&packetLength);
        HANDLE_RET_CODE(hr, "GetNextPacketSize", done);
    }

done:
    if (FAILED(hr))
        throw ChunkRecordingFailure(__FILE__, __FUNCTION__, __LINE__, ("Failed to record a chunk: " + std::to_string(hr)).c_str());
}

WAVEFORMATEX *WASAPIAudioRecorder::getFormat() {
    return format;
}

WASAPIAudioRecorder::~WASAPIAudioRecorder() {
    HRESULT hr = OK;

    if (pAudioClient != nullptr)
        pAudioClient->Stop();
    SAFE_RELEASE(enumerator)
    CoTaskMemFree(format);
    SAFE_RELEASE(pCaptureClient)
    SAFE_RELEASE(pAudioClient)
    SAFE_RELEASE(recorder)

    CoUninitialize();

    spdlog::info("Closed recorder with error code: {}", hr);
}
