#pragma once

#include <Audioclient.h>

#include <iostream>

using namespace std;

typedef int errcode_t;

#define OK 0

#define ERROR_FILE_ACCESS 130

#define ERROR_START_CAPTURE 520

void showWasapiErrorMessage(HRESULT er);
