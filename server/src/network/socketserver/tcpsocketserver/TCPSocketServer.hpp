#pragma once
#include <vector>

#include "ErrorCodes.hpp"
#include "network/socketserver/ISocketServer.hpp"

class TCPSocketServer final : public ISocketServer {
public:
    ~TCPSocketServer() override;

    errcode_t init(const SocketConfig &config) override;

    errcode_t start() override;

    errcode_t stop() override;

    void showHostInfo() override;

    errcode_t sendMessage(const BYTE *message, UINT32 size) override;

    errcode_t recvAll(void *data, int size) override;
    errcode_t recvMessage(BYTE *buffer, UINT32 bufferSize, UINT32 &bytes_received) override;
    errcode_t recvMessage(std::vector<BYTE> &message) override;

private:
    SOCKET _listenSocket{};
    SOCKET _clientSocket{};
    sockaddr_in _serverAddr{};
    sockaddr_in _clientAddr{};

    WSADATA wsaData = {};

    int _port{};
};
