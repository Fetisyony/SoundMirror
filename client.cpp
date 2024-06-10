#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#define PORT 20000
#define SERVER_IP "192.168.0.101" // Replace with server's IP address

int main(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        (void)puts("WSAStartup failed");
        return 1;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        // Handle error
        (void)puts("sock failed");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        // Handle error
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connected to server!\n");

    // receive something
    char buffer[1024];
    int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesRead == SOCKET_ERROR) {
        // Handle error
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {
        buffer[bytesRead] = '\0';
        printf("Received: %s\n", buffer);
    }

    // send
    const char* message = "Hello from client!";
    int bytesSent = send(sock, message, strlen(message), 0);
    if (bytesSent == SOCKET_ERROR) {
        // Handle error
        closesocket(sock);
        WSACleanup();
        return 1;
    } else {
        printf("Sent: %s\n", message);
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}
