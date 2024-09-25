#pragma once

#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include <iostream>

using namespace std;

#define OK 0

#define DISCONNECTED 430

#define ERROR_FILE_ACCESS 130

#define ERROR_START_CAPTURE 520

void showWasapiErrorMessage(HRESULT er);
