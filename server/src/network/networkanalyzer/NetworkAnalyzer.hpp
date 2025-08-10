#pragma once
#include <string>

class NetworkAnalyzer {
public:
    static bool IsPrivateIp(const char *ipAddress);

    static std::string getLocalLanIpAddress();
};
