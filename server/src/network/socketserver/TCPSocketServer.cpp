#include "TCPSocketServer.hpp"

#include <network/server_errors.hpp>

TCPSocketServer::~TCPSocketServer() {
    stop();
}

errcode_t TCPSocketServer::init(int port) {
    errcode_t rc = OK;

    _port = port;

    _serverAddr.sin_family = AF_INET; // IPv4
    _serverAddr.sin_port = htons(_port); // Convert to network byte order
    _serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

    WSADATA wsaData;
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
    char hostname[1024];
    gethostname(hostname, 1024);
    hostent *host = gethostbyname(hostname);
    char *ip = inet_ntoa(*reinterpret_cast<struct in_addr *>(host->h_addr_list[0]));
    printf("Server listening on port %d\n", _port);
    printf("Host IP: %s\n", ip);
}

errcode_t TCPSocketServer::sendMessage(BYTE *message, UINT32 size) {
    int bytes_send = send(_clientSocket, reinterpret_cast<const char *>(message), size, 0);
    if (bytes_send == SOCKET_ERROR) {
        cout << "Error while sending" << endl;
        return DISCONNECTED;
    }
    return OK;
}
