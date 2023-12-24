#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include <optional>
#include <cerrno>
#include <system_error>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdint>
#include <cassert>
#include <csignal>
#include <thread>
#include <chrono>

#include "./tools.h"
#include "./sockets.h"

int main(int argc, char* argv[]) {
    // Configurar manejadores de señales
    struct sigaction sa{};
    sa.sa_handler = tools::signal_handler;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
    sigaction(SIGQUIT, &sa, nullptr);

    std::optional<tools::program_options> options = tools::parse_args(argc, argv);
    if (!options) {
        return EXIT_FAILURE; // Error de parse
    }

    // Si se ha pedido ayuda, la mostramos
    if (options->show_help) {
        tools::print_usage();
        return 0;
    }

    // Si NETCP_PORT o NETCP_IP no están definidas o no existen, le asignamos valores por defecto
    if (std::getenv("NETCP_PORT") == nullptr) {
        setenv("NETCP_PORT", "8080", 0);
    }
    if (std::getenv("NETCP_IP") == nullptr) {
        setenv("NETCP_IP", "127.0.0.1", 0);
    }

    // Obtener valores de las variables de entorno NETCP_PORT y NETCP_IP
    std::string netcp_port_env = std::getenv("NETCP_PORT");
    std::string netcp_ip_env = std::getenv("NETCP_IP");

    // Si se ha pedido un fichero o comando, proseguimos
    if (!options->filename.empty()) {
            if (options->mode_listen) {
                // Creamos el socket
                Sockets socket(netcp_ip_env, netcp_port_env);
                std::error_code receive_error = socket.netcp_receive_file(options->filename);
                if (receive_error) {
                    std::cerr << "netcp: error al recibir el archivo: " << receive_error.message() << "\n";
                    return EXIT_FAILURE;
                }
            } else if (options->mode_send) {
                // Creamos el socket
                Sockets socket(netcp_ip_env, "0");
                // Configurar la dirección de destino antes de entrar al bucle
                auto destinationAddressOpt = socket.make_ip_address(netcp_ip_env, std::stoi(netcp_port_env));
                if (!destinationAddressOpt.has_value()) {
                    std::cerr << "netcp: error al configurar la dirección de destino\n";
                    return EXIT_FAILURE;
                }
                auto destinationAddress = destinationAddressOpt.value();  // Extraer el valor de std::optional
                std::error_code send_error = socket.netcp_send_file(options->filename, destinationAddress);
                if (send_error) {
                    std::cerr << "netcp: error al enviar el archivo: " << send_error.message() << "\n";
                    return EXIT_FAILURE;
                }
            } else {
                std::cerr << "netcp: modo no especificado\n";
                return EXIT_FAILURE;
            }
    }

    return EXIT_SUCCESS;
}
