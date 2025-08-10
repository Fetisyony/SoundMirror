#pragma once
#include <memory>

class ISocketServer;

class TimeSyncService {
public:
    explicit TimeSyncService(int port);

    void sync();

private:
    int _port;
    std::shared_ptr<ISocketServer> _server{};

    int64_t getCurrentTimeMillis();
};
