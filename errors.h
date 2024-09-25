#pragma once

#include <initguid.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmreg.h>

#include <iostream>

using namespace std;

#define OK 0
#define DISCONNECTED 1

void showWasapiErrorMessage(HRESULT er);
