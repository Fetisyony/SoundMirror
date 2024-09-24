#include "server.h"

void show_hostinfo() {
    char hostname[1024];
    gethostname(hostname, 1024);
    struct hostent *host = gethostbyname(hostname);
    char* ip = inet_ntoa(*(struct in_addr *)host->h_addr_list[0]);
    printf("Server listening on port %d\n", PORT);
    printf("Host IP: %s\n", ip);
}


int convert_endianess_and_send(BYTE* pData, UINT32 nFrames, WAVEFORMATEX *format) {
    int rc = OK;
    UINT32 bytes_captured = format->nBlockAlign * nFrames;
    // Convert the audio data from little-endian to big-endian
    swap_sound_endianess(pData, nFrames, format);

    // Here you can save the data, send it to another process, etc.
    rc = send_to_client(pData, bytes_captured);
    return rc;
}


int init_server_tcp(SOCKET &listenSocket) {
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

int init_server(SOCKET &listenSocket, sockaddr_in &clientAddr) {
    int rc = OK;

    WSADATA wsaData;
    if (rc == OK && WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        rc = ERROR_WSAFAILED;
    }

    if (rc == OK) {
        listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

    if (rc == OK)
        show_hostinfo();

    if (rc == OK) {
        int fromlen = sizeof(clientAddr);
        char recvbuf[1024];
        int recvBytes = recvfrom(listenSocket, recvbuf, 1024, 0,
                                (sockaddr*)&clientAddr, &fromlen);
        if (recvBytes == SOCKET_ERROR) {
            printf("recvfrom failed: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            rc = ERROR_ACCEPT_FAILED;
        } else {
            printf("Received: %s\n", recvbuf);
        }
        int sendBytes = sendto(listenSocket, "Hello, Bitch!", strlen("Hello, Bitch!") + 1, 0,
                        (sockaddr*)&clientAddr, sizeof(clientAddr));
    }

    cout << "Accepted" << endl;

    return rc;
}


int send_to_client(BYTE *message, UINT32 size) {
    // Send data to the client
    return send_to_client_tcp(message, size);

    int sendBytes = sendto(listenSocket, reinterpret_cast<const char *>(message), size, 0,
                        (sockaddr*)&clientAddr, sizeof(clientAddr));
    if (sendBytes == SOCKET_ERROR) {
        printf("sendto failed: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return SOCKET_ERROR;
    }
    
    return OK;
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

int send_to_client_tcp(BYTE *message, UINT32 size) {
    int bytes_send = send(clientSocket, reinterpret_cast<const char *>(message), size, 0);
    if (bytes_send == SOCKET_ERROR) {
        cout << "Error while sending" << endl;
        return DISCONNECTED;
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
