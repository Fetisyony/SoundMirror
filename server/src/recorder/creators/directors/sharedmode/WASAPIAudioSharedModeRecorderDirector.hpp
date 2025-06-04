#pragma once
#include <memory>

#include "recorder/WASAPIAudioRecorder.hpp"
#include "recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp"

class WASAPIAudioSharedModeRecorderDirector {
public:
    explicit WASAPIAudioSharedModeRecorderDirector(HANDLE eventHandle);
    std::shared_ptr<WASAPIAudioRecorder> create();
private:
    HANDLE _eventHandle;
    std::shared_ptr<WASAPIAudioRecorderBuilder> _builder = std::make_shared<WASAPIAudioRecorderBuilder>();
};
