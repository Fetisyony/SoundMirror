#include <stdio.h>
#include <iostream>

#include <winsock2.h>
#include <windows.h>

#include "errors.h"

#define PORT 20000

#define ERROR_WSAFAILED 2
#define ERROR_SOCKETFAILED 3
#define ERROR_BIND_FAILED 4
#define ERROR_LISTEN_FAILED 5
#define ERROR_ACCEPT_FAILED 6
#define ERROR_SEND_FAILED 7

extern SOCKET clientSocket;

void SetNonBlocking(SOCKET socket) {
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) != NO_ERROR) {
        std::cerr << "Failed to set non-blocking mode: " << WSAGetLastError() << std::endl;
    }
}

int init_server(SOCKET &listenSocket) {
    int rc = OK;

    WSADATA wsaData;
    if (rc == OK && WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        rc = ERROR_WSAFAILED;
    }

    if (rc == OK) {
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET)
            rc = ERROR_SOCKETFAILED;
    }

    struct sockaddr_in serverAddr;
    if (rc == OK) {
        serverAddr.sin_family = AF_INET;  // IPv4
        serverAddr.sin_port = htons(PORT);  // Convert to network byte order
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    }

    if (rc == OK && bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        rc = ERROR_BIND_FAILED;
    }

    if (rc == OK && listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        rc = ERROR_LISTEN_FAILED;
    }

    if (rc == OK) {
        // showing host ip
        char hostname[1024];
        gethostname(hostname, 1024);
        struct hostent *host = gethostbyname(hostname);
        char* ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
        printf("Server listening on port %d\n", PORT);
        printf("Host IP: %s\n", ip);
    }

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    if (rc == OK) {
        clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
            rc = ERROR_ACCEPT_FAILED;
    }

    std::cout << "Accepted" << std::endl;

    return rc;
}

int send_to_client(BYTE *message, UINT32 size) {
    int rc = OK;

    if (rc == OK && send(clientSocket, reinterpret_cast<const char*>(message), size, 0) == SOCKET_ERROR) {
        rc = ERROR_SEND_FAILED;
        puts("Failed");
    }

    return rc;
}

int run(void) {
    int rc = OK;

    SOCKET listenSocket;

    rc = init_server(listenSocket);
    std::cout << "Server inited with code " << rc << std::endl;

    if (rc == OK)
        rc = send_to_client((unsigned char *)"hello", 5);

    std::cout << "Server sended with code " << rc << std::endl;

    (void)closesocket(clientSocket);
    (void)closesocket(listenSocket);
    WSACleanup();

    return rc;
}