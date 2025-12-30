#pragma once
#include <winsock2.h>
#include <string>
#include <ws2tcpip.h>

class NetworkAnalyzer {
public:
    static bool IsPrivateIp(const char *ipAddress);

    static std::string getLocalLanIpAddress();

    static std::pair<std::string, int> sockAddrToString(const sockaddr_in &addr) {
        char buf[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
        return {std::string(buf), ntohs(addr.sin_port)};
    }
};
