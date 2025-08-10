#pragma once

#define HANDLE_RET_CODE(hr, message, label) if (FAILED(hr)) { \
    cout << "Error: " << message << " - failed (" << (long)hr << ")" << endl; \
    goto label; \
}
