#pragma once
#include <atomic>
#include <memory>
#include <vector>

#include "remote/sockets/socketserver/IAcceptor.hpp"

namespace sync_data {
    inline std::atomic<int64_t> timestampMilliseconds{-1};
    inline std::atomic<int64_t> ticks{-1};
    inline std::atomic<int64_t> frequency{-1};
}

class TimeSyncService {
public:
    explicit TimeSyncService(int port);

    void syncConversation();

private:
    int _syncsNumber = 1;
    int _port;
    std::shared_ptr<IAcceptor> _server{};

    std::string _syncRequestExpected = "SYNC";
    std::vector<char> _syncRequest;

    void sync(const std::shared_ptr<IConnection> &conn);
};
