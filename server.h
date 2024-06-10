#pragma once

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


int init_server(SOCKET &listenSocket);

int send_to_client(BYTE *message, UINT32 size);
