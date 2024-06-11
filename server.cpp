#include "server.h"

extern SOCKET clientSocket;
extern UINT64 passed;

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

bool get_false(UINT64 tmp) {
    passed += tmp;
    return false;
}

int close_socket() {
    return OK;
}

unsigned short swap_endianess(unsigned short value) {
    return (value << 8) | (value >> 8);
}

int send_short(unsigned short input_little_end) {
    // Prepare buffer to send
    unsigned short input_big_end = swap_endianess(input_little_end);
    const char* buf = reinterpret_cast<const char*>(&input_big_end);
    
    // Now you can use the send function to send the data over the network
    int bytesSent = send(clientSocket, buf, sizeof(input_big_end), 0);
    if (bytesSent == SOCKET_ERROR) {
        cout << "Error sending data." << input_little_end << endl;
        return SOCKET_ERROR;
    }
    return OK;
}

int announce_format(WAVEFORMATEX *format) {
    send_short(format->nChannels);
    send_short(format->wBitsPerSample / 8);
    send_short((unsigned short)format->nSamplesPerSec);
    
    return OK;
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
