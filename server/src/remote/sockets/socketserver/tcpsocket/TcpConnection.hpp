#pragma once
#include <atomic>
#include <vector>

#include "ErrorCodes.hpp"
#include "remote/sockets/socketserver/IAcceptor.hpp"
#include "remote/sockets/socketserver/IConnection.hpp"

class TcpConnection final : public IConnection {
public:
    TcpConnection(SOCKET s, sockaddr_in peer);
    ~TcpConnection() noexcept override;

    ssize_t send(const void* data, size_t len) override;
    ssize_t recv(void* buf, size_t len) override;
    ssize_t recvAll(void *data, int size) override;
    ssize_t recvMessage(byte *buffer, int32_t bufferSize, int32_t &bytes_received) override;
    ssize_t recvMessage(std::vector<byte> &message) override;
    void close() override;
    std::string peerAddress() const override;

private:
    static std::string lastErrMsg();

    SOCKET _socket;
    sockaddr_in _peer;
    std::atomic<bool> _closed;
};
