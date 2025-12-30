#pragma once
#include <memory>

#include "remote/soundstreamingservice/recorder/WASAPIAudioRecorder.hpp"
#include "remote/soundstreamingservice/recorder/creators/builder/WASAPIAudioRecorderBuilder.hpp"


class WASAPIAudioExclusiveModeRecorderDirector {
public:
    explicit WASAPIAudioExclusiveModeRecorderDirector(HANDLE eventHandle);

    std::shared_ptr<WASAPIAudioRecorder> create();

private:
    HANDLE _eventHandle;
    std::shared_ptr<WASAPIAudioRecorderBuilder> _builder = std::make_shared<WASAPIAudioRecorderBuilder>();
};
