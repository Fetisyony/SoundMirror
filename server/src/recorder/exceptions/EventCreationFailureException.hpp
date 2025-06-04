#pragma once
#include "BaseRecorderException.hpp"

class EventCreationFailureException final : public BaseRecorderException {
public:
    EventCreationFailureException(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Exception when trying to create an event"
    ) : BaseRecorderException(filename, funcName, line, text) {}
};