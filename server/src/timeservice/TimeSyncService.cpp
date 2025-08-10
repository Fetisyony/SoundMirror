#include "TimeSyncService.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

#include "../network/socketserver/tcpsocketserver/TCPSocketServer.hpp"
#include "timeservice/SyncException.hpp"

TimeSyncService::TimeSyncService(int port) : _port(port) {
    _server = std::make_shared<TCPSocketServer>();
    auto config = SocketConfig(_port);
    _server->init(config);
}

void TimeSyncService::sync() {
    auto rc = _server->start();
    if (rc != OK) {
        _server->stop();
        throw SyncException(__FILE__, __FUNCTION__, __LINE__, "Failed to start server");
    }

    [[maybe_unused]] constexpr char syncRequestExpected[] = "SYNC";
    char syncRequest[sizeof(syncRequestExpected)] = {};
    rc = _server->recvAll(syncRequest, strlen(syncRequestExpected));
    if (rc != OK || strncmp(syncRequest, syncRequestExpected, 4) != 0) {
        char message[100];
        snprintf(
            message,
            sizeof(message),
            "Failed to get timestamp sync request from the client: rc = %d, syncRequest = %s", rc, syncRequest
        );
        _server->stop();
        throw SyncException(__FILE__, __FUNCTION__, __LINE__, message);
    }

    int64_t arrivalTime = getCurrentTimeMillis();

    const size_t totalSize = 2 * sizeof(int64_t);
    BYTE buffer[totalSize];

    auto writeLongToBufferLE = [](BYTE* buf, int64_t val) {
        for (int i = 0; i < 8; ++i) {
            buf[i] = static_cast<BYTE>((val >> (i * 8)) & 0xFF);
        }
    };

    writeLongToBufferLE(buffer, arrivalTime);

    int64_t departureServerTime = getCurrentTimeMillis();
    writeLongToBufferLE(buffer + sizeof(int64_t), departureServerTime);

    rc = _server->sendMessage(buffer, sizeof(buffer));
    if (rc != OK) {
        _server->stop();
        throw SyncException(__FILE__, __FUNCTION__, __LINE__, "Failed to send message to the client");
    }
    std::cout << "Server: Sent timestamps to client." << std::endl;

    _server->stop();
}

int64_t TimeSyncService::getCurrentTimeMillis() {
    const auto now = std::chrono::system_clock::now();
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    std::cout << "Timestamp: " << value << std::endl;
    return value;
}
