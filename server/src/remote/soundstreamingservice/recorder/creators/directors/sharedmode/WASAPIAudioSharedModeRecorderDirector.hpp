#pragma once
#include <memory>

#include "remote/soundstreamingservice/recorder/IAudioRecorder.hpp"
#include "remote/soundstreamingservice/recorder/WASAPIAudioRecorder.hpp"
#include "remote/soundstreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp"

class WASAPIAudioSharedModeRecorderDirector {
public:
    explicit WASAPIAudioSharedModeRecorderDirector(HANDLE eventHandle);
    std::shared_ptr<IAudioRecorder> create();
private:
    HANDLE _eventHandle;
    std::shared_ptr<WASAPIAudioRecorderBuilder> _builder = std::make_shared<WASAPIAudioRecorderBuilder>();
};
