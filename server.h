#pragma once

#include <stdio.h>
#include <iostream>

#include <windows.h>

#include "server_errors.h"

#define PORT 20000

using std::cout;
using std::endl;

typedef unsigned char BYTE;

extern SOCKET clientSocket;
extern UINT64 passed;

int init_server(SOCKET &listenSocket);

int send_to_client(BYTE *message, UINT32 size);

int announce_format(WAVEFORMATEX *format);

int send_short(unsigned short input_little_end);

unsigned short swap_endianess(unsigned short value);

bool get_false(UINT64 tmp);

int close_socket();

int run_test(void);
