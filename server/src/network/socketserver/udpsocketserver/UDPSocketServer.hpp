#pragma once
#include "../ISocketServer.hpp"
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")

class UDPSocketServer : public ISocketServer {
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

private:
    SOCKET _socket{};
    sockaddr_in _serverAddr{};
    sockaddr_in _clientAddr{};
    std::string _hostName{};
    bool _isInitialized{};
};