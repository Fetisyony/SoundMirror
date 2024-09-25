#pragma once

#include <iostream>
#include <string>

#include <windows.h>
#include "../mathutils/converters.h"

#include "server_errors.h"
#include "../ErrorsConfig.h"

using namespace std;

#define PORT 20000

typedef unsigned char BYTE;


class ServerUDP {
protected:
    SOCKET serverSocket;
    sockaddr_in serverAddr, clientAddr;
    string serverAddressStr;
    int port;
public:
    ServerUDP(string address, int port);
    virtual ~ServerUDP();

    virtual int init() = 0;

    virtual int start() = 0;
    virtual int stop() = 0;

    virtual void showHostinfo() = 0;
    virtual int send_to_client(BYTE *message, UINT32 size) = 0;
};

class ServerTCP {
protected:
    SOCKET listenSocket;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    sockaddr_in clientAddr;

    int port;

public:
    ServerTCP(int port);
    ~ServerTCP();

    virtual int start();

    virtual int init();

    virtual int stop();

    virtual void showHostinfo();

    virtual int send_to_client(BYTE *message, UINT32 size);
};


class StreamServer : public ServerUDP {
public:
    StreamServer(string address, int port);
    ~StreamServer();

    int init() override;

    int start() override;
    int stop() override;

    void showHostinfo() override;
    int send_to_client(BYTE *message, UINT32 size) override;

    int announce_format(WAVEFORMATEX *format);

    int send_short(unsigned short input_little_end);

    int convert_endianess_and_send(BYTE* pData, UINT32 nFrames, WAVEFORMATEX *format);
};

