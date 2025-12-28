#include "UDPSocketClient.hpp"

UDPSocketClient::UDPSocketClient() noexcept
    : _socket(INVALID_SOCKET), _isInitialized(false), _wsaStarted(false)
{
    ZeroMemory(&_serverAddr, sizeof(_serverAddr));
}

UDPSocketClient::~UDPSocketClient() {
    closeSocket();
    if (_wsaStarted) {
        WSACleanup();
        _wsaStarted = false;
    }
}

errcode_t UDPSocketClient::init(const std::string &server_ip, uint16_t server_port) {
    WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (wsaErr != 0) {
        std::cerr << "[udp_client] WSAStartup failed: " << wsaErr << std::endl;
        return SOCKET_ERROR;
    }
    _wsaStarted = true;

    // Create UDP socket
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET) {
        std::cerr << "[udp_client] socket() failed: " << WSAGetLastError() << std::endl;
        return SOCKET_ERROR;
    }

    // Prepare server address
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(server_port);

    // Convert IP string to binary form
    int conv = inet_pton(AF_INET, server_ip.c_str(), &_serverAddr.sin_addr);
    if (conv <= 0) {
        // conv == 0 -> invalid string, conv < 0 -> error
        std::cerr << "[udp_client] inet_pton failed for IP '" << server_ip
                  << "' (result=" << conv << "), WSAErr=" << WSAGetLastError() << std::endl;
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        return SOCKET_ERROR;
    }

    _isInitialized = true;
    std::cout << "[udp_client] Initialized to send to " << server_ip << ":" << server_port << std::endl;
    return OK;
}

errcode_t UDPSocketClient::sendMessage(const BYTE *data, UINT32 size) {
    if (!_isInitialized) return SOCKET_ERROR;
    if (!data || size == 0) return SOCKET_ERROR;
    if (size > MAX_UDP_PAYLOAD) return SOCKET_ERROR;

    int sent = sendto(_socket,
                      reinterpret_cast<const char*>(data),
                      static_cast<int>(size),
                      0,
                      reinterpret_cast<const sockaddr*>(&_serverAddr),
                      static_cast<int>(sizeof(_serverAddr)));

    if (sent == SOCKET_ERROR) {
        int err = WSAGetLastError();
        std::cerr << "[udp_client] sendto failed: " << err << std::endl;
        return SOCKET_ERROR;
    }

    if (sent != static_cast<int>(size)) {
        // Unlikely for UDP, but handle the case
        std::cerr << "[udp_client] warning: sent " << sent << " bytes out of requested " << size << std::endl;
    }

    return OK;
}

void UDPSocketClient::closeSocket() noexcept {
    if (_socket != INVALID_SOCKET) {
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        _isInitialized = false;
        std::cout << "[udp_client] Socket closed." << std::endl;
    }
}
