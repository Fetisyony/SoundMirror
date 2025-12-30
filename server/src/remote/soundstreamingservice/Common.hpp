#pragma once

#include <condition_variable>

namespace common {
    inline std::mutex consumerProducerMutex;
    inline std::condition_variable cv;
}
