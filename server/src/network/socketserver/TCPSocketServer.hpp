#pragma once
#include "ErrorCodes.hpp"
#include "ISocketServer.hpp"

class TCPSocketServer final : public ISocketServer {
public:
    ~TCPSocketServer() override;

    errcode_t init(int port) override;

    errcode_t start() override;

    errcode_t stop() override;

    void showHostInfo() override;

    errcode_t sendMessage(BYTE *message, UINT32 size) override;

private:
    SOCKET _listenSocket{};
    SOCKET _clientSocket{};
    sockaddr_in _serverAddr{};
    sockaddr_in _clientAddr{};

    int _port{};
};
