#include "UDPSocketClient.hpp"

#include <cstdint>
#include <iostream>
#include <ws2tcpip.h>

#include "remote/sockets/exceptions/NetworkExceptions.hpp"

UDPSocketClient::UDPSocketClient(const std::string &server_ip, uint16_t server_port) {
    _peerStr = server_ip + ":" + std::to_string(server_port);

    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (wsaErr != 0) {
        spdlog::error("[udp_client] WSAStartup failed: {}", wsaErr);
        throw network::ResourceException("WSAStartup", _peerStr);
    }
    _wsaStarted = true;

    _socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET) {
        spdlog::error("[udp_client] socket() failed: {}", WSAGetLastError());
        throw network::ResourceException("socket", _peerStr);
    }

    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(server_port);
    int conv = inet_pton(AF_INET, server_ip.c_str(), &_serverAddr.sin_addr);
    if (conv <= 0) {
        spdlog::error("[udp_client] inet_pton failed for IP '{}' (result={}), WSAErr={}",
                      server_ip, conv, WSAGetLastError());
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        throw network::AddressResolutionException("inet_pton", _peerStr);
    }

    _isInitialized = true;
    spdlog::info("[udp_client] Initialized to send to {}", _peerStr);
}

UDPSocketClient::~UDPSocketClient() {
    closeSocket();

    if (_wsaStarted) {
        int rc = WSACleanup();
        if (rc != 0) {
            spdlog::warn("[udp_client] WSACleanup returned {}", rc);
        } else {
            spdlog::info("[udp_client] WSACleanup done");
        }
        _wsaStarted = false;
    }
}

void UDPSocketClient::sendMessage(const uint8_t *data, uint32_t size) {
    if (!_isInitialized) {
        spdlog::error("[udp_client] sendMessage called but client is not initialized");
        throw network::SendException("sendMessage", _peerStr);
    }
    if (!data || size == 0) {
        spdlog::error("[udp_client] sendMessage invalid args: data={}, size={}", (void*)data, size);
        throw network::SendException("sendMessage", _peerStr);
    }
    if (size > MAX_UDP_PAYLOAD) {
        spdlog::error("[udp_client] sendMessage payload too large: {} > {}", size, MAX_UDP_PAYLOAD);
        throw network::SendException("sendMessage", _peerStr);
    }

    int sent = ::sendto(_socket,
                        reinterpret_cast<const char*>(data),
                        static_cast<int>(size),
                        0,
                        reinterpret_cast<const sockaddr*>(&_serverAddr),
                        static_cast<int>(sizeof(_serverAddr)));

    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        spdlog::error("[udp_client] sendto failed (to {}): WSAErr={}", _peerStr, err);
        throw network::SendException("sendto", _peerStr);
    }

    if (static_cast<uint32_t>(sent) != size) {
        spdlog::warn("[udp_client] partial send: sent {} bytes out of requested {}", sent, size);
    } else {
        spdlog::debug("[udp_client] sent {} bytes to {}", sent, _peerStr);
    }
}

void UDPSocketClient::closeSocket() noexcept {
    if (_socket != INVALID_SOCKET) {
        int rc = closesocket(_socket);
        if (rc != 0) {
            spdlog::warn("[udp_client] closesocket returned {}", rc);
        } else {
            spdlog::info("[udp_client] Socket closed.");
        }
        _socket = INVALID_SOCKET;
        _isInitialized = false;
    }
}
