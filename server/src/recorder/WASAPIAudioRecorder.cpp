#include "recorder/WASAPIAudioRecorder.hpp"

#include <functional>

#include "ErrorHandling.hpp"
#include "ErrorCodes.hpp"
#include "exceptions/ChunkRecordingFailure.hpp"
#include "exceptions/StartRecordingFailure.hpp"


bool WASAPIAudioRecorder::isEnough(const UINT64 received, const UINT64 bytesInSecond, const long double chunkSeconds) {
    return received >= bytesInSecond * chunkSeconds;
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

    bytesReceived = 0;

    hr = pCaptureClient->GetNextPacketSize(&packetLength);
    HANDLE_RET_CODE(hr, "GetNextPacketSize", done);

    while (bytesReceived < bufferLimit) {
        hr = pCaptureClient->GetBuffer(&captureBuffer, &nFrames, &flags, nullptr, nullptr);
        HANDLE_RET_CODE(hr, "GetBuffer", done);

        if (nFrames == 0) {
            // No more data currently pending.
            pCaptureClient->ReleaseBuffer(0);
            break;
        }

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            // In shared mode, flags==SILENT means “zero data.”
            // For future use
        }

        // nFramesThatFit <= (bufferLimit - received) / format->nBlockAlign
        UINT64 nFramesThatFit = min((bufferLimit - bytesReceived) / format->nBlockAlign, static_cast<UINT64>(nFrames));
        UINT64 nBytesThatFit = nFramesThatFit * format->nBlockAlign;
        memcpy(destBuffer + bytesReceived, captureBuffer, nBytesThatFit);
        bytesReceived += nBytesThatFit;

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

    cout << "Closed recorder with error code: " << hr << endl;
}
