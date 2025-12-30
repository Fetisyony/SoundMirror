#pragma once
#include <string>
#include <stdint.h>


struct SocketConfig {
    // --- Common ---
    uint16_t server_port = 0;

    // --- For UDP ---
    std::string client_ip = "";
    uint16_t client_port = 0;
};
