#pragma once

#include <winsock2.h>
#include <string>

#include <spdlog/spdlog.h>

class UDPSocketClient {
public:
    UDPSocketClient(const std::string &server_ip, uint16_t server_port);
    ~UDPSocketClient();

    void sendMessage(const uint8_t *data, uint32_t size);
    void closeSocket() noexcept;

    UDPSocketClient(const UDPSocketClient&) = delete;
    UDPSocketClient& operator=(const UDPSocketClient&) = delete;

private:
    SOCKET _socket { INVALID_SOCKET };
    sockaddr_in _serverAddr {};
    bool _wsaStarted { false };
    bool _isInitialized { false };
    std::string _peerStr;

    static constexpr uint32_t MAX_UDP_PAYLOAD = 65507u;
};
