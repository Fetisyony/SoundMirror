#include "server.h"

ServerUDP::ServerUDP(std::string address, int port) : serverAddressStr(address), port(port) {
}

ServerUDP::~ServerUDP() {}

StreamServer::StreamServer(std::string address, int port)
    : ServerUDP(address, port) {
    serverAddr.sin_family = AF_INET;  // IPv4
    serverAddr.sin_port = htons(port);  // Convert to network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
}

StreamServer::~StreamServer() {
    stop();
}

int StreamServer::init() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return ERROR_WSAFAILED;
    }

    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET)
        return ERROR_SOCKETFAILED;
    
    return OK;
}

int StreamServer::start() {
    int rc = OK;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        return ERROR_BIND_FAILED;
    }

    showHostinfo();

    int fromlen = sizeof(clientAddr);
    char recvbuf[1024];
    int recvBytes = recvfrom(
                                serverSocket,
                                recvbuf,
                                1024,
                                0,
                                (sockaddr*)&clientAddr,
                                &fromlen
                            );

    if (recvBytes == SOCKET_ERROR) {
        printf("[StreamServer::start] recvfrom failed: %d\n", WSAGetLastError());
        return ERROR_ACCEPT_FAILED;
    }

    printf("[StreamServer::start] Received: %s\n", recvbuf);
    int sendBytes = sendto(
                            serverSocket,
                            "Hello, Bitch!",
                            strlen("Hello, Bitch!") + 1,
                            0,
                            (sockaddr*)&clientAddr,
                            sizeof(clientAddr)
                            );

    cout << "[StreamServer::start] Accepted" << endl;

    return rc;
}

int StreamServer::stop() {
    closesocket(serverSocket);

    WSACleanup();

    return OK;
}

int StreamServer::send_to_client(BYTE *message, UINT32 size) {
    int sendBytes = sendto(
                            serverSocket,
                            reinterpret_cast<const char *>(message),
                            size,
                            0,
                            (sockaddr*)&clientAddr,
                            sizeof(clientAddr)
                            );

    if (sendBytes == SOCKET_ERROR) {
        printf("[StreamServer] sendto failed: %d\n", WSAGetLastError());
        return SOCKET_ERROR;
    }
    
    return OK;
}

int StreamServer::convert_endianess_and_send(BYTE* pData, UINT32 nFrames, WAVEFORMATEX *format) {
    int rc = OK;

    UINT32 bytes_captured = format->nBlockAlign * nFrames;
    // Convert the audio data from little-endian to big-endian
    swap_sound_endianess(pData, nFrames, format);

    // Here you can save the data, send it to another process, etc.
    rc = send_to_client(pData, bytes_captured);
    return rc;
}


int StreamServer::announce_format(WAVEFORMATEX *format) {
    int rc = OK;

    if (rc == OK)
        rc = send_short(format->nChannels);

    if (rc == OK)
        rc = send_short(format->wBitsPerSample / 8);

    if (rc == OK)
        rc = send_short((unsigned short)format->nSamplesPerSec);
    
    return OK;
}

int StreamServer::send_short(unsigned short input_little_end) {
    unsigned short input_big_end = swap_endianess(input_little_end);
    BYTE *buf = reinterpret_cast<BYTE *>(&input_big_end);
    
    int bytesSent = send_to_client(buf, sizeof(input_big_end));
    if (bytesSent == SOCKET_ERROR) {
        cout << "Error sending data." << input_little_end << endl;
        return SOCKET_ERROR;
    }
    return OK;
}

void StreamServer::showHostinfo() {
    char hostname[1024];
    gethostname(hostname, 1024);
    struct hostent *host = gethostbyname(hostname);
    char* ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
    printf("Server listening on port %d\n", PORT);
    printf("Host IP: %s\n", ip);
}

// ===================================================================

ServerTCP::ServerTCP(string address, int port) {
    serverAddr.sin_family = AF_INET;  // IPv4
    serverAddr.sin_port = htons(PORT);  // Convert to network byte order
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces

    int rc = init();
    if (rc != OK) {
        cout << "Error happened while initializing server socket." << endl;
    }
}

ServerTCP::~ServerTCP() {
    stop();
}

int ServerTCP::init() {
    int rc = OK;

    WSADATA wsaData;
    if (rc == OK && WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return ERROR_WSAFAILED;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
        return ERROR_SOCKETFAILED;

    return rc;
}

int ServerTCP::start() {
    int rc = OK;

    if (rc == OK && bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        rc = ERROR_BIND_FAILED;
    }

    if (rc == OK && listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        rc = ERROR_LISTEN_FAILED;
    }

    if (rc == OK)
        showHostinfo();

    int clientAddrSize = sizeof(clientAddr);
    if (rc == OK) {
        clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
            rc = ERROR_ACCEPT_FAILED;
    }

    cout << "[serverTCP] Accepted someone" << endl;

    return rc;
}

int ServerTCP::stop() {
    closesocket(clientSocket);
    closesocket(listenSocket);

    WSACleanup();
    
    return OK;
}

void ServerTCP::showHostinfo() {
    char hostname[1024];
    gethostname(hostname, 1024);
    struct hostent *host = gethostbyname(hostname);
    char* ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
    printf("Server listening on port %d\n", PORT);
    printf("Host IP: %s\n", ip);
}

int ServerTCP::send_to_client(BYTE *message, UINT32 size) {
    int bytes_send = send(clientSocket, reinterpret_cast<const char *>(message), size, 0);
    if (bytes_send == SOCKET_ERROR) {
        cout << "Error while sending" << endl;
        return DISCONNECTED;
    }
    return OK;
}
