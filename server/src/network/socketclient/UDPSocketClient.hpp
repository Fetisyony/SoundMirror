#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

#include "ErrorCodes.hpp"


class UDPSocketClient {
public:
    UDPSocketClient() noexcept;
    ~UDPSocketClient();

    // init with server IP (dotted quad) and port; must be called before sendMessage
    errcode_t init(const std::string &server_ip, uint16_t server_port);

    // send raw buffer as one UDP datagram
    errcode_t sendMessage(const BYTE *data, UINT32 size);

    // convenience overload
    errcode_t sendMessage(const std::vector<BYTE> &data) {
        return sendMessage(data.data(), static_cast<UINT32>(data.size()));
    }

    // close socket early (optional)
    void closeSocket() noexcept;

    bool isInitialized() const noexcept { return _isInitialized; }

private:
    SOCKET _socket;
    sockaddr_in _serverAddr;
    bool _isInitialized;
    bool _wsaStarted;
    static constexpr UINT32 MAX_UDP_PAYLOAD = 65507; // safe max UDP payload
};
