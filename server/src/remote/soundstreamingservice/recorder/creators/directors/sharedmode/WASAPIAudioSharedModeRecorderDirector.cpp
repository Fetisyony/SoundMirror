#include "remote/soundstreamingservice/recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp"
#include "ErrorCodes.hpp"
#include "remote/soundstreamingservice/recorder/ErrorHandling.hpp"
#include "remote/soundstreamingservice/recorder/MonoDownmixAudioRecorderDecorator.hpp"
#include "remote/soundstreamingservice/recorder/ResamplingAudioRecorderDecorator.hpp"
#include "remote/soundstreamingservice/recorder/SoxrResampler.hpp"
#include "remote/soundstreamingservice/recorder/exceptions/RecorderCreationFailed.hpp"

WASAPIAudioSharedModeRecorderDirector::WASAPIAudioSharedModeRecorderDirector(HANDLE eventHandle)
    : _eventHandle(eventHandle) {}

std::shared_ptr<IAudioRecorder> WASAPIAudioSharedModeRecorderDirector::create() {
    HRESULT hr = OK;
    std::shared_ptr<IAudioRecorder> recorder = nullptr;
    int targetSr = 16000;

    hr = _builder->initEnumerator();
    HANDLE_RET_CODE(hr, "initEnumerator", failed);

    hr = _builder->initializeLoopbackRecorder();
    HANDLE_RET_CODE(hr, "initializeLoopbackRecorder", failed);

    hr = _builder->activateClient();
    HANDLE_RET_CODE(hr, "activateClient", failed);

    hr = _builder->initializeSharedModeFormat();
    HANDLE_RET_CODE(hr, "initializeFormat", failed);

    hr = _builder->initializeSharedClient(SECONDS_IN_SHARED_BUFFER);
    HANDLE_RET_CODE(hr, "initializeSharedClient", failed);

    hr = _builder->setEventHandle(_eventHandle);
    HANDLE_RET_CODE(hr, "setEventHandle", failed);

    hr = _builder->getService();
    HANDLE_RET_CODE(hr, "getService", failed);

    recorder = _builder->build();

    if (recorder->getFormat()->nChannels == 2) {
        recorder = std::make_shared<MonoDownmixAudioRecorderDecorator>(recorder);
    }

    if (recorder->getFormat()->nSamplesPerSec != targetSr) {
        auto resampler = std::make_shared<SoxrResampler>(
            recorder->getFormat()->nSamplesPerSec,
            targetSr,
            recorder->getFormat()->nChannels,
            recorder->getFormat()->wBitsPerSample
        );

        return std::make_shared<ResamplingAudioRecorderDecorator>(recorder, resampler, targetSr);
    }

    return recorder;

failed:
    throw RecorderCreationFailed(__FILE__, __FUNCTION__, __LINE__, ("Failed to create a recorder: " + std::to_string(hr)).c_str());
}
