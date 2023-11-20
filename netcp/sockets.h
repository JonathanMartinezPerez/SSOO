#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <optional>
#include <cerrno>
#include <system_error>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdint>
#include <cassert>
#include <expected>
#include <cstring>
#include <string_view>


using make_socket_result = std::expected<int, std::error_code>;

class Sockets {
public:
    Sockets(const std::optional<sockaddr_in>& address = std::nullopt) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);

        if (sockfd == -1) {
            throw std::system_error(errno, std::generic_category(), "Error al crear el socket");
        }

        if (address) {
            if (bind(sockfd, reinterpret_cast<const sockaddr*>(&address.value()), sizeof(sockaddr_in)) == -1) {
                close(sockfd);
                throw std::system_error(errno, std::generic_category(), "Error al realizar el bind del socket");
            }
        }
    }

    ~Sockets() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }

    std::error_code sendTo(const std::vector<uint8_t>& message, const sockaddr_in& address) const {
        ssize_t bytesSent = sendto(sockfd, message.data(), message.size(), 0,
                                   reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in));

        if (bytesSent == -1) {
            return std::error_code(errno, std::generic_category());
        }

        return std::error_code(); // Éxito
    }

    
    static std::optional<sockaddr_in> make_ip_address(const std::optional<std::string>& ip_address, uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (ip_address.has_value()) {
            if (inet_pton(AF_INET, ip_address->c_str(), &addr.sin_addr) <= 0) {
                return std::nullopt; // Error en la conversión de la dirección IP
            }
        } else {
            addr.sin_addr.s_addr = INADDR_ANY;
        }

        return addr;
    }

    int getSocketFD() const {
        return sockfd;
    }

private:
    int sockfd;
};





