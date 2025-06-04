#pragma once
#include "BaseRecorderException.hpp"

class StartRecordingFailure final : public BaseRecorderException {
public:
    StartRecordingFailure(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Failed to start recording"
    ) : BaseRecorderException(filename, funcName, line, text) {}
};