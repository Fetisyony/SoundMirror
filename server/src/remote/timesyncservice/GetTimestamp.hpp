#pragma once

#include <Windows.h>

namespace get_time_service {
    inline int64_t getCurrentTicksCount() {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        return qpc.QuadPart;
    }

    inline int64_t getCurrentTimeMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

    inline int64_t getMillisecondsBetweenTicks(int64_t startTicks, int64_t endTicks, int64_t frequency) {
        int64_t elapsedTicks = endTicks - startTicks;
        return (elapsedTicks * 1000) / frequency;
    }
}
