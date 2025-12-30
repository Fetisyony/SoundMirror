#pragma once

#include <system_error>
#include <string>
#include <winsock2.h>

namespace network {

inline std::error_code last_error_code() noexcept {
#ifdef _WIN32
    return std::error_code(WSAGetLastError(), std::system_category());
#else
    return std::error_code(errno, std::generic_category());
#endif
}

class SocketException : public std::system_error {
public:
    SocketException(std::error_code ec,
                    std::string operation,
                    std::string peer = {})
        : std::system_error(ec),
          operation_(std::move(operation)),
          peer_(std::move(peer)),
          message_(build_message())
    {}

    explicit SocketException(std::string operation,
                             std::string peer = {})
        : SocketException(last_error_code(), std::move(operation), std::move(peer))
    {}

    const char* what() const noexcept override { return message_.c_str(); }

    const std::string& operation() const noexcept { return operation_; }
    const std::string& peer() const noexcept { return peer_; }
    std::error_code code() const noexcept { return std::system_error::code(); }

private:
    std::string build_message() const {
        std::string msg = operation_;
        if (!peer_.empty()) {
            msg += " [" + peer_ + "]";
        }
        msg += ": ";
        msg += code().message();
        return msg;
    }

    std::string operation_;
    std::string peer_;
    std::string message_;
};

struct AddressResolutionException : SocketException { using SocketException::SocketException; };
struct BindException             : SocketException { using SocketException::SocketException; };
struct ListenException           : SocketException { using SocketException::SocketException; };
struct AcceptException           : SocketException { using SocketException::SocketException; };
struct ConnectException          : SocketException { using SocketException::SocketException; };
struct SendException             : SocketException { using SocketException::SocketException; };
struct ReceiveException          : SocketException { using SocketException::SocketException; };
struct DatagramException         : SocketException { using SocketException::SocketException; };
struct TimeoutException          : SocketException { using SocketException::SocketException; };
struct ClosedException           : SocketException { using SocketException::SocketException; };
struct ProtocolException         : SocketException { using SocketException::SocketException; };
struct ResourceException         : SocketException { using SocketException::SocketException; };
struct PermissionException       : SocketException { using SocketException::SocketException; };
struct AddressInUseException     : SocketException { using SocketException::SocketException; };

} // namespace network
