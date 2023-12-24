#pragma once

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


using make_socket_result = std::expected<int, std::error_code>;

//Creamos una clase Sockets la cual hace el make del socket en el constructor y el close en el destructor
class Sockets {
    public:
        //Constructor con el make
        Sockets (const std::string& ip, const std::string& port) {
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);

            if (sockfd == -1) {
                throw std::system_error(errno, std::generic_category(), "Error al crear el socket");
            }

            configure_socket_options(ip, port);
            if (bind(sockfd, reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in)) == -1) {
                close(sockfd);
                throw std::system_error(errno, std::generic_category(), "Error al realizar el bind del socket");
            }
        }
        
        //Destructor con el close
        ~Sockets() {
            if (sockfd != -1) {
                close(sockfd);
            }
        }

        //Función para mandar el mentaje tras leerlo
        std::error_code sendTo(const std::vector<uint8_t>& message, const sockaddr_in& address) const {
            ssize_t bytesSent = sendto(sockfd, message.data(), message.size(), 0,
                                    reinterpret_cast<const sockaddr*>(&address), sizeof(sockaddr_in));

            if (bytesSent == -1) {
                return std::make_error_code(static_cast<std::errc>(errno));
            }

            return std::error_code(); // Éxito
        }

        std::error_code receiveFrom(std::vector<uint8_t>& buffer, sockaddr_in& sourceAddress) {
            socklen_t sourceAddressLength = sizeof(sourceAddress);
            ssize_t bytesReceived = recvfrom(sockfd, buffer.data(), buffer.size(), 0,
                                            reinterpret_cast<sockaddr*>(&sourceAddress), &sourceAddressLength);

            if (bytesReceived < 0) {
                return std::make_error_code(static_cast<std::errc>(errno));
            }
            buffer.resize(bytesReceived);

            return std::error_code(); // Éxito
        }

        //Función para crear las direcciones de origen y destino
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

        // Función para enviar un archivo
        std::error_code netcp_send_file(const std::string& filename, const sockaddr_in& destinationAddress) {
            int file_fd = open(filename.c_str(), O_RDONLY);
            if (file_fd == -1) {
                std::cerr << "netcp: no se puede abrir '" << filename << "': " << strerror(errno) << "\n";
                return std::make_error_code(static_cast<std::errc>(errno));
            }

            while (!tools::quit_requested) {
                std::vector<uint8_t> buffer(1024); // Tamaño del fragmento, ajusta según necesidades
                ssize_t bytes_read = read(file_fd, buffer.data(), buffer.size());

                if (bytes_read < 0) {
                    std::error_code error(errno, std::generic_category());
                    close(file_fd);
                    return error;
                } else if (bytes_read == 0) {
                    // Enviar un mensaje vacío al final del archivo
                    std::error_code send_error = send_empty_message(destinationAddress);
                    if (send_error) {
                        close(file_fd);
                        return send_error;
                    }
                    break;
                }

                buffer.resize(bytes_read);

                // Enviar fragmento usando send_to
                std::error_code send_error = sendTo(buffer, destinationAddress);
                if (send_error) {
                    close(file_fd);
                    return send_error;
                }
            }

            close(file_fd);
            return std::error_code(); // Éxito
        }

        // Función para recibir un archivo
        std::error_code netcp_receive_file(const std::string& filename) {
            int file_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (file_fd == -1) {
                std::cerr << "netcp: no se puede abrir o crear '" << filename << "': " << strerror(errno) << "\n";
                return std::make_error_code(static_cast<std::errc>(errno));
            }

            while (!tools::quit_requested) {
                std::vector<uint8_t> buffer(1024); // Tamaño del fragmento, ajusta según necesidades
                std::cout << "netcp: esperando a recibir...\n";
                std::error_code receive_error = receiveFrom(buffer, destinationAddress);
                std::cout << "netcp: recibidos " << buffer.size() << " bytes\n";
                empty_message_received = buffer.empty();
                if (receive_error) {
                    // Manejar error en la recepción del fragmento
                    close(file_fd);
                    return receive_error;
                }

                // Escribir fragmento en el archivo
                std::error_code write_error = tools::write_file(file_fd, buffer);
                if (write_error) {
                    // Manejar error en la escritura del archivo
                    close(file_fd);
                    return write_error;
                }

                if (empty_message_received) {
                    // Salir del bucle al recibir el mensaje vacío
                    break;
                }
            }
 
            close(file_fd);
            return std::error_code(); // Éxito
        }

        void configure_socket_options (const std::string& ip, const std::string& port) {
            address.sin_family = AF_INET;

            std::string local_ip = ip, local_port = port;

            if (local_ip.empty()) {
                local_ip = "127.0.0.1";
            }

            if (local_port.empty()) {
                local_port = "8080";
            }

            uint16_t local_integer_port = std::stoi(local_port);

            address = make_ip_address(local_ip, local_integer_port).value();
            std::cout << "Configured port: " << local_integer_port << std::endl;
            std::cout << "Configured IP: " << local_ip << std::endl;
        }

        // Función para setear el puerto
        void setPort(uint16_t port) {
            address.sin_port = htons(port);
        }

        // Función para setear la dirección IP
        void setIPAddress(const sockaddr_in& ip_address) {
            address.sin_addr = ip_address.sin_addr;
        }

    private:
        int sockfd;
        bool empty_message_received;
        sockaddr_in address;
        sockaddr_in destinationAddress;
        bool last_fragment;
        // Función auxiliar para enviar un mensaje vacío
        std::error_code send_empty_message(const sockaddr_in& destinationAddress) {
            std::vector<uint8_t> empty_message(0);
            return sendTo(empty_message, destinationAddress);
        }
};
