#include "UDPSocketServer.hpp"
#include <iostream>
#include <ws2tcpip.h>

#include "network/socketserver/exceptions/NotImplementedException.hpp"
#include "network/server_errors.hpp"

errcode_t UDPSocketServer::init(const SocketConfig &config) {
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET) {
        return ERROR_SOCKETFAILED;
    }

    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(config.server_port);
    _serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_socket, reinterpret_cast<sockaddr*>(&_serverAddr), sizeof(_serverAddr)) == SOCKET_ERROR) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return ERROR_SOCKETFAILED;
    }

    if (config.client_ip.empty() || config.client_port == 0) {
        return ERROR_SOCKETFAILED;
    }
    _clientAddr.sin_family = AF_INET;
    _clientAddr.sin_port = htons(config.client_port);
    inet_pton(AF_INET, config.client_ip.c_str(), &_clientAddr.sin_addr);

    _isInitialized = true;
    std::cout << "[serverUDP] Initialized and bound to port " << config.server_port << std::endl;
    
    char host[256];
    if (gethostname(host, sizeof(host)) == 0) {
        _hostName = host;
    }

    return OK;
}

errcode_t UDPSocketServer::start() {
    if (!_isInitialized) {
        return ERROR_SOCKETFAILED;
    }
    
    std::cout << "[serverUDP] Server is active" << std::endl;
    showHostInfo();
    return OK;
}

errcode_t UDPSocketServer::stop() {
    if (_socket != INVALID_SOCKET) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        _isInitialized = false;
        std::cout << "[serverUDP] Socket closed." << std::endl;
    }
    return OK;
}

void UDPSocketServer::showHostInfo() {
    if (_hostName.empty()) {
        std::cout << "[serverUDP] Host info not available." << std::endl;
        return;
    }
    std::cout << "----------- UDP Server Info -----------" << std::endl;
    std::cout << "  Host Name: " << _hostName << std::endl;
    std::cout << "  Port: " << ntohs(_serverAddr.sin_port) << std::endl;
    std::cout << "-------------------------------------" << std::endl;
}

errcode_t UDPSocketServer::sendMessage(const BYTE *message, UINT32 size) {
    if (_socket == INVALID_SOCKET) return ERROR_SOCKETFAILED;

    int bytes_sent = sendto(_socket,
                            reinterpret_cast<const char*>(message),
                            size,
                            0,
                            reinterpret_cast<const sockaddr*>(&_clientAddr),
                            sizeof(_clientAddr));

    if (bytes_sent == SOCKET_ERROR) {
        return ERROR_SEND_FAILED;
    }

    return OK;
}

errcode_t UDPSocketServer::recvAll(void *data, int size) {
    throw NotImplementedException(__FILE__, __FUNCTION__, __LINE__, "recvAll method not implemented");
}


errcode_t UDPSocketServer::recvMessage(std::vector<BYTE> &message) {
    throw NotImplementedException(__FILE__, __FUNCTION__, __LINE__, "recvMessage method not implemented");
}

errcode_t UDPSocketServer::recvMessage(BYTE *buffer, UINT32 bufferSize, UINT32 &bytes_received) {
    throw NotImplementedException(__FILE__, __FUNCTION__, __LINE__, "recvMessage method not implemented");
}

UDPSocketServer::~UDPSocketServer() {
    if (_socket != INVALID_SOCKET) {
        UDPSocketServer::stop();
    }
    WSACleanup();
}
