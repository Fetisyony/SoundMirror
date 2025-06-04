#include "recorder/creators/directors/sharedmode/WASAPIAudioSharedModeRecorderDirector.hpp"
#include "ErrorCodes.hpp"
#include "recorder/exceptions/RecorderCreationFailed.hpp"

WASAPIAudioSharedModeRecorderDirector::WASAPIAudioSharedModeRecorderDirector(HANDLE eventHandle)
    : _eventHandle(eventHandle) {}

std::shared_ptr<WASAPIAudioRecorder> WASAPIAudioSharedModeRecorderDirector::create() {
    HRESULT hr = OK;

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

    return _builder->build();

failed:
    throw RecorderCreationFailed(__FILE__, __FUNCTION__, __LINE__, ("Failed to create a recorder: " + std::to_string(hr)).c_str());
}
