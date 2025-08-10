#include "WASAPIAudioExclusiveModeRecorderDirector.hpp"

#include "ErrorCodes.hpp"

WASAPIAudioExclusiveModeRecorderDirector::WASAPIAudioExclusiveModeRecorderDirector(HANDLE eventHandle) : _eventHandle(eventHandle) {}

std::shared_ptr<WASAPIAudioRecorder> WASAPIAudioExclusiveModeRecorderDirector::create() {
    HRESULT hr = OK;

    hr = _builder->initEnumerator();
    HANDLE_RET_CODE(hr, "initEnumerator", failed);

    hr = _builder->initializeMicrophoneRecorder();
    HANDLE_RET_CODE(hr, "initializeLoopbackRecorder", failed);

    hr = _builder->activateClient();
    HANDLE_RET_CODE(hr, "activateClient", failed);

    hr = _builder->initializeExclusiveModeFormat();
    HANDLE_RET_CODE(hr, "initializeFormat", failed);

    hr = _builder->initializeExclusiveClient(SECONDS_IN_SHARED_BUFFER);
    HANDLE_RET_CODE(hr, "initializeExclusiveClient", failed);

    hr = _builder->setEventHandle(_eventHandle);
    HANDLE_RET_CODE(hr, "setEventHandle", failed);

    hr = _builder->getService();
    HANDLE_RET_CODE(hr, "getService", failed);

    return _builder->build();

failed:
    std::cout << "Director failed :(" << std::endl;
    return nullptr;
}
