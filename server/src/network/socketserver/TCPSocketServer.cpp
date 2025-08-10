#include <winsock2.h>
#include "TCPSocketServer.hpp"
#include <iphlpapi.h>
#include <network/server_errors.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <ws2tcpip.h>

#include "../networkanalyzer/NetworkAnalyzer.hpp"

errcode_t TCPSocketServer::init(int port) {
    errcode_t rc = OK;

    _port = port;

    _serverAddr.sin_family = AF_INET; // IPv4
    _serverAddr.sin_port = htons(_port); // Convert to network byte order
    _serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return ERROR_WSAFAILED;

    _listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSocket == INVALID_SOCKET)
        return ERROR_SOCKETFAILED;

    return rc;
}

errcode_t TCPSocketServer::start() {
    errcode_t rc = OK;

    if (bind(_listenSocket, reinterpret_cast<sockaddr *>(&_serverAddr), sizeof(_serverAddr)) == SOCKET_ERROR) {
        rc = ERROR_BIND_FAILED;
    }

    if (rc == OK && listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        rc = ERROR_LISTEN_FAILED;
    }

    if (rc == OK)
        showHostInfo();

    int clientAddrSize = sizeof(_clientAddr);
    if (rc == OK) {
        _clientSocket = accept(_listenSocket, reinterpret_cast<sockaddr *>(&_clientAddr), &clientAddrSize);
        if (_clientSocket == INVALID_SOCKET)
            rc = ERROR_ACCEPT_FAILED;
    }

    cout << "[serverTCP] Accepted someone" << endl;

    return rc;
}

errcode_t TCPSocketServer::stop() {
    closesocket(_clientSocket);
    closesocket(_listenSocket);

    WSACleanup();

    return OK;
}

void TCPSocketServer::showHostInfo() {
    printf("Server listening on port %d\n", _port);
    const std::string ip = NetworkAnalyzer::getLocalLanIpAddress();
    if (!ip.empty()) {
        printf("Host IP (LAN): %s\n", ip.c_str());
    } else {
        printf("Could not determine local LAN IP address. Server listening on port %d\n", _port);
    }
}

errcode_t TCPSocketServer::sendMessage(BYTE *message, UINT32 size) {
    int bytes_send = send(_clientSocket, reinterpret_cast<const char *>(message), size, 0);
    if (bytes_send == SOCKET_ERROR) {
        cout << "Error while sending" << endl;
        return DISCONNECTED;
    }
    return OK;
}

errcode_t TCPSocketServer::recvAll(void *data, int size) {
    char *buffer = static_cast<char *>(data);
    int total_bytes_received = 0;
    while (total_bytes_received < size) {
        int bytes_received = recv(_clientSocket, buffer + total_bytes_received, size - total_bytes_received, 0);
        if (bytes_received == -1) {
            std::cerr << "Error during recvAll" << std::endl;
            return DISCONNECTED;
        }
        if (bytes_received == 0) {
            std::cout << "Client disconnected during recvAll." << std::endl;
            return DISCONNECTED;
        }
        total_bytes_received += bytes_received;
    }
    return OK;
}

errcode_t TCPSocketServer::recvMessage(std::vector<BYTE> &message) {
    UINT32 network_size = 0;
    if (recvAll(&network_size, sizeof(network_size)) != OK) {
        return DISCONNECTED;
    }

    UINT32 size = ntohl(network_size);

    constexpr UINT32 MAX_MSG_SIZE = 10 * 1024 * 1024;
    if (size > MAX_MSG_SIZE) {
        std::cerr << "Error: Message size " << size << " exceeds limit." << std::endl;
        return DISCONNECTED;
    }

    message.resize(size);
    if (recvAll(message.data(), size) != OK) {
        return DISCONNECTED;
    }

    return OK;
}

errcode_t TCPSocketServer::recvMessage(BYTE *buffer, UINT32 bufferSize, UINT32 &bytes_received) {
    int bytes = recv(_clientSocket, reinterpret_cast<char *>(buffer), bufferSize, 0);

    if (bytes > 0) {
        bytes_received = bytes;
        return OK;
    }
    if (bytes == 0) {
        std::cout << "Client disconnected gracefully." << std::endl;
        bytes_received = 0;
        return DISCONNECTED;
    }

    std::cout << "recv failed with error." << std::endl;
    bytes_received = 0;
    return DISCONNECTED;
}

TCPSocketServer::~TCPSocketServer() {
    stop();
}
