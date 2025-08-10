#pragma once
#include "exceptions/BaseException.hpp"

class SyncException final : public BaseException {
public:
    SyncException(
        const char *filename,
        const char *funcName,
        const int line,
        const char *text = "Failed to sync"
    ) : BaseException(filename, funcName, line, text) {}
};
