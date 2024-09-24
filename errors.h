#pragma once

#include <winsock2.h>
#include <windows.h>
#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include <iostream>

using namespace std;

#define OK 0
#define DISCONNECTED 1

void show_error_init_client(HRESULT er);
