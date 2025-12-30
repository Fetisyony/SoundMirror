#include "TimeSyncService.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <windows.h>

#include "GetTimestamp.hpp"
#include "remote/sockets/byteorder/ByteOrder.hpp"
#include "remote/sockets/socketserver/SocketConfig.hpp"
#include "remote/sockets/socketserver/tcpsocket/TCPAcceptor.hpp"
#include "remote/sockets/socketserver/tcpsocket/TcpConnection.hpp"
#include "remote/timesyncservice/SyncException.hpp"
#include "spdlog/spdlog.h"


TimeSyncService::TimeSyncService(int port) : _port(port) {
    auto config = SocketConfig(_port);
    _server = std::make_shared<TcpAcceptor>(_port);

    _syncRequest.resize(_syncRequestExpected.size(), 0);
}

void TimeSyncService::syncConversation() {
    std::shared_ptr<IConnection> conn = _server->accept();

    for (int i = 0; i < _syncsNumber; ++i)
        sync(conn);

    spdlog::info("Synced");
    _server->stop();
}

void TimeSyncService::sync(const std::shared_ptr<IConnection> &conn) {
    conn->recvAll(_syncRequest.data(), _syncRequestExpected.size());
    int64_t arrivalTime = get_time_service::getCurrentTimeMs();
    auto arrivalTicks = get_time_service::getCurrentTicksCount();

    if (!std::equal(_syncRequestExpected.begin(), _syncRequestExpected.end(), _syncRequest.begin())) {
        char message[100];
        snprintf(
            message,
            sizeof(message),
            "Failed to get timestamp sync request from the client: syncRequest = %s", _syncRequest.data()
        );
        conn->close();
        throw SyncException(__FILE__, __FUNCTION__, __LINE__, message);
    }

    BYTE buffer[2 * sizeof(int64_t)];
    byteorder::writeLongToBufferLE(buffer, arrivalTime);

    int64_t departureServerTime = get_time_service::getCurrentTimeMs();
    auto departureTicks = get_time_service::getCurrentTicksCount();
    byteorder::writeLongToBufferLE(buffer + sizeof(int64_t), departureServerTime);

    conn->send(buffer, sizeof(buffer));
    spdlog::info("Server: Sent timestamps to client.");

    sync_data::timestampMilliseconds = (departureServerTime + arrivalTime) / 2;
    sync_data::ticks = (departureTicks + arrivalTicks) / 2;

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    sync_data::frequency = freq.QuadPart;
}
