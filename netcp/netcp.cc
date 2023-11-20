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


#include "./tools.h"

std::optional<sockaddr_in> make_ip_address(const std::optional<std::string>& ip_address, uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (ip_address.has_value()) {
        if (inet_pton(AF_INET, ip_address->c_str(), &address.sin_addr) <= 0) {
            return std::nullopt; // Error en la conversión de la dirección IP
        }
    } else {
        address.sin_addr.s_addr = INADDR_ANY;
    }

    return address;
}

using make_socket_result = std::expected<int, std::error_code>;

make_socket_result make_socket(const std::optional<sockaddr_in>& address = std::nullopt) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        return std::unexpected(std::error_code(errno, std::generic_category()));
    }

    if (address) {
        if (bind(sockfd, reinterpret_cast<const sockaddr*>(&address.value()), sizeof(sockaddr_in)) == -1) {
            close(sockfd);
            return std::unexpected(std::error_code(errno, std::generic_category()));
        }
    }

    return sockfd;
}

std::error_code send_to(int fd, const std::vector<uint8_t>& message, const sockaddr_in& address) {
    ssize_t bytesSent = sendto(fd, message.data(), message.size(), 0,
                               reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in));

    if (bytesSent == -1) {
        return std::error_code(errno, std::generic_category());
    }

    return std::error_code(); // Éxito
}

int main(int argc, char* argv[]) {
    
    std::optional<tools::program_options> options = tools::parse_args(argc, argv);
    if (!options) {
        return 1; // Error al parsear los argumentos
    }

    if (options->show_help) {
        tools::print_usage();
        return 0;
    }

    if (!options->filename.empty()) {
        // Abre el archivo
        int file_fd = open(options->filename.c_str(), O_RDONLY);
        if (file_fd == -1) {
            std::cerr << "netcp: no se puede abrir '" << options->filename << "': " << strerror(errno) << "\n";
            return 1;
        }

        // Obtiene información sobre el archivo para verificar el tamaño
        struct stat file_info;
        if (fstat(file_fd, &file_info) == -1) {
            std::cerr << "netcp: no se puede obtener información sobre '" << options->filename << "': " << strerror(errno) << "\n";
            close(file_fd);
            return 1;
        }

        // Verifica el tamaño del archivo
        if (file_info.st_size > 1024) {
            std::cerr << "netcp: el tamaño del archivo '" << options->filename << "' es superior a 1 KiB\n";
            close(file_fd);
            return 1;
        }

        // Lee el contenido del archivo en un buffer
        std::vector<uint8_t> buffer(file_info.st_size);
        std::error_code read_error = tools::read_file(file_fd, buffer);
        close(file_fd);

        if (read_error) {
            std::cerr << "netcp: error al leer el archivo '" << options->filename << "': " << read_error.message() << "\n";
            return 1;
        }

        // Crea la dirección IP y el socket
        std::optional<sockaddr_in> originAddress = make_ip_address("0.0.0.0", 8081);
        std::optional<sockaddr_in> destinationAddress = make_ip_address("0.0.0.0", 8080);
        make_socket_result socket_result = make_socket(originAddress);

        if (!socket_result) {
            std::cerr << "netcp: error al crear el socket: " << socket_result.error().message() << "\n";
            return 1;
        }

        // Envia el contenido del archivo por UDP
        std::error_code send_error = send_to(socket_result.value(), buffer, destinationAddress.value());

        if (send_error) {
            std::cerr << "netcp: error al enviar datos por UDP: " << send_error.message() << "\n";
            close(socket_result.value());
            return 1;
        }

        // Todo ha ido bien
        close(socket_result.value());
        return EXIT_SUCCESS;
    }

    return 0;
}
