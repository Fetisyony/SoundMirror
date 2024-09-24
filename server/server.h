#pragma once

#include <iostream>

#include <thread>
#include <windows.h>
#include <string>

#include "server_errors.h"

using namespace std;

#define PORT 20000

typedef unsigned char BYTE;

extern SOCKET listenSocket;
extern UINT64 passed;


// implements server socket for udp for one client
class ServerUDP {
    SOCKET listenSocket;
    sockaddr_in clientAddr;
public:
    ServerUDP(string address, int port);
    ~ServerUDP();

    int start();

    int stop();

    void show_hostinfo();

    int send_to_client(BYTE *message, UINT32 size);
};

class ServerTCP {
    SOCKET listenSocket;
    SOCKET clientSocket;
    sockaddr_in clientAddr;

    ServerTCP(string address, int port);
    ~ServerTCP();

    int start();

    int stop();

    void show_hostinfo();

    int send_to_client(BYTE *message, UINT32 size);
    
    int send_short(unsigned short input_little_end);
};



