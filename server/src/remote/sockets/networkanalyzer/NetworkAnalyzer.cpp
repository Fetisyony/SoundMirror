#include "NetworkAnalyzer.hpp"

#include <winsock2.h>
#include <iostream>
#include <ipifcons.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "spdlog/spdlog.h"


bool NetworkAnalyzer::IsPrivateIp(const char *ipAddress) {
    if (!ipAddress) return false;

    IN_ADDR addr;
    if (InetPtonA(AF_INET, ipAddress, &addr) != 1) {
        return false;
    }

    // convert to host byte order
    unsigned int ip = ntohl(addr.S_un.S_addr);

    // Class A: 10.0.0.0 to 10.255.255.255 (10.0.0.0/8)
    if ((ip >= 0x0A000000) && (ip <= 0x0AFFFFFF)) return true;

    // Class B: 172.16.0.0 to 172.31.255.255 (172.16.0.0/12)
    if ((ip >= 0xAC100000) && (ip <= 0xAC1FFFFF)) return true;

    // Class C: 192.168.0.0 to 192.168.255.255 (192.168.0.0/16)
    if ((ip >= 0xC0A80000) && (ip <= 0xC0A8FFFF)) return true;

    return false;
}

std::string NetworkAnalyzer::getLocalLanIpAddress() {
    std::string lanIp;
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = nullptr;
    ULONG ulOutBufLen = 15000;
    DWORD dwRetVal = 0;

    do {
        pAdapterAddresses = static_cast<IP_ADAPTER_ADDRESSES *>(malloc(ulOutBufLen));
        if (pAdapterAddresses == nullptr) {
            spdlog::error("Memory allocation failed for GetAdaptersAddresses");
            return "";
        }

        dwRetVal = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
            nullptr,
            pAdapterAddresses,
            &ulOutBufLen
        );

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            free(pAdapterAddresses);
            pAdapterAddresses = nullptr;
        } else if (dwRetVal != NO_ERROR) {
            spdlog::error("GetAdaptersAddresses failed with error: {}", dwRetVal);

            free(pAdapterAddresses);
            return "";
        }
    } while (dwRetVal == ERROR_BUFFER_OVERFLOW);

    for (PIP_ADAPTER_ADDRESSES pAdapter = pAdapterAddresses; pAdapter != nullptr; pAdapter = pAdapter->Next) {
        // We are interested in physical LAN (Ethernet) or Wi-Fi adapters that are operational
        if (pAdapter->OperStatus == IfOperStatusUp &&
            (pAdapter->IfType == MIB_IF_TYPE_ETHERNET || pAdapter->IfType == IF_TYPE_IEEE80211)) {
            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pAdapter->FirstUnicastAddress; pUnicast != nullptr;
                 pUnicast = pUnicast->Next) {
                if (pUnicast->Address.lpSockaddr->sa_family == AF_INET) {
                    // IPv4 address
                    auto *pIpAddr = reinterpret_cast<sockaddr_in *>(pUnicast->Address.lpSockaddr);
                    char ipStr[INET_ADDRSTRLEN];
                    if (InetNtopA(AF_INET, &(pIpAddr->sin_addr), ipStr, INET_ADDRSTRLEN)) {
                        if (strcmp(ipStr, "127.0.0.1") == 0) {
                            continue;
                        }

                        if (IsPrivateIp(ipStr)) {
                            lanIp = ipStr;
                            break; // Found a suitable LAN IP, take the first one
                        }
                    }
                }
            }
        }
        if (!lanIp.empty()) {
            break; // Found an IP, exit
        }
    }

    if (pAdapterAddresses) {
        free(pAdapterAddresses);
    }
    return lanIp;
}
