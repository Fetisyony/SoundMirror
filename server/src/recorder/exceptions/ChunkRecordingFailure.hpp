#pragma once
#include "BaseRecorderException.hpp"

class ChunkRecordingFailure final : public BaseRecorderException {
public:
    ChunkRecordingFailure(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Failed to record a chunk"
    ) : BaseRecorderException(filename, funcName, line, text) {}
};