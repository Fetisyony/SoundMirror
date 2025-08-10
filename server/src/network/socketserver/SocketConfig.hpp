#pragma once
#include <string>
#include <cstdint>

struct SocketConfig {
    // --- Common ---
    uint16_t server_port = 0;

    // --- UDP Specific ---
    std::string client_ip;
    uint16_t client_port = 0;
};
