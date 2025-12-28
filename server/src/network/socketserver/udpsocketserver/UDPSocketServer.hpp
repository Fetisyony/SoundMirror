#pragma once
#include <winsock2.h>
#include "network/socketserver/ISocketServer.hpp"
#include <string>

class UDPSocketServer final : public ISocketServer {
public:
    ~UDPSocketServer() override;

    errcode_t init(const SocketConfig &config) override;
    errcode_t start() override;
    errcode_t stop() override;
    void showHostInfo() override;

    errcode_t sendMessage(const BYTE *message, UINT32 size) override;

    errcode_t recvAll(void *data, int size) override;
    errcode_t recvMessage(std::vector<BYTE> &message) override;
    errcode_t recvMessage(BYTE *buffer, UINT32 bufferSize, UINT32 &bytes_received) override;

    sockaddr_in &getClient() override;

private:
    SOCKET _socket{};
    sockaddr_in _serverAddr{};
    sockaddr_in _clientAddr{};
    std::string _hostName{};
    bool _isInitialized{};
};