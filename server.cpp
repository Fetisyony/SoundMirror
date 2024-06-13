#include "server.h"

void show_hostinfo() {
    char hostname[1024];
    gethostname(hostname, 1024);
    struct hostent *host = gethostbyname(hostname);
    char* ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
    printf("Server listening on port %d\n", PORT);
    printf("Host IP: %s\n", ip);
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

    if (rc == OK)
        show_hostinfo();

    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    if (rc == OK) {
        clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
            rc = ERROR_ACCEPT_FAILED;
    }

    cout << "Accepted" << endl;

    return rc;
}

int send_to_client(BYTE *message, UINT32 size) {
    int bytes_send = send(clientSocket, reinterpret_cast<const char *>(message), size, 0);
    if (bytes_send == SOCKET_ERROR) {
        cout << "Error while sending" << endl;
        return DISCONNECTED;
    }
    return OK;
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
    unsigned short input_big_end = swap_endianess(input_little_end);
    BYTE *buf = reinterpret_cast<BYTE *>(&input_big_end);
    
    int bytesSent = send_to_client(buf, sizeof(input_big_end));
    if (bytesSent == SOCKET_ERROR) {
        cout << "Error sending data." << input_little_end << endl;
        return SOCKET_ERROR;
    }
    return OK;
}

int announce_format(WAVEFORMATEX *format) {
    int rc = OK;

    if (rc == OK)
        rc = send_short(format->nChannels);

    if (rc == OK)
        rc = send_short(format->wBitsPerSample / 8);

    if (rc == OK)
        rc = send_short((unsigned short)format->nSamplesPerSec);
    
    return OK;
}

int run_test(void) {
    int rc = OK;

    SOCKET listenSocket;

    rc = init_server(listenSocket);
    cout << "Server inited with code " << rc << endl;

    if (rc == OK)
        rc = send_to_client((BYTE *)"hello", 5);

    cout << "Server sended with code " << rc << endl;

    (void)closesocket(clientSocket);
    (void)closesocket(listenSocket);
    WSACleanup();

    return rc;
}
