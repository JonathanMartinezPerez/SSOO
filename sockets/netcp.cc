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
#include "./sockets.h"

int main(int argc, char* argv[]) {
    std::optional<tools::program_options> options = tools::parse_args(argc, argv);
    if (!options) {
        return 1; // Error de parse
    }

    //Si se ha pedido ayuda, la mostramos
    if (options->show_help) {
        tools::print_usage();
        return 0;
    }

    //Si se ha pedido un fichero proseguimos
    if (!options->filename.empty()) {
        int file_fd = open(options->filename.c_str(), O_RDONLY);
        if (file_fd == -1) {
            std::cerr << "netcp: no se puede abrir '" << options->filename << "': " << strerror(errno) << "\n";
            return 1;
        }

        //Obtenemos la información del fichero
        struct stat file_info;
        if (fstat(file_fd, &file_info) == -1) {
            std::cerr << "netcp: no se puede obtener información sobre '" << options->filename << "': " << strerror(errno) << "\n";
            close(file_fd);
            return 1;
        }

        //Comprobamos que el tamaño del fichero no sea superior a 1 KiB
        if (file_info.st_size > 1024) {
            std::cerr << "netcp: el tamaño del archivo '" << options->filename << "' es superior a 1 KiB\n";
            close(file_fd);
            return 1;
        }

        //Creamos un buffer con el tamaño del fichero, leemos el fichero y lo cerramos
        std::vector<uint8_t> buffer(file_info.st_size);
        std::error_code read_error = tools::read_file(file_fd, buffer);
        close(file_fd);

        if (read_error) {
            std::cerr << "netcp: error al leer el archivo '" << options->filename << "': " << read_error.message() << "\n";
            return 1;
        }

        //Creamos las direcciones de origen y destino
        std::optional<sockaddr_in> originAddress = Sockets::make_ip_address("0.0.0.0", 8081);
        std::optional<sockaddr_in> destinationAddress = Sockets::make_ip_address("127.0.0.1", 8080);

        //Creamos el socket y enviamos el mensaje
        try {
            Sockets socketWrapper(originAddress);

            std::error_code send_error = socketWrapper.sendTo(buffer, destinationAddress.value());

            if (send_error) {
                std::cerr << "netcp: error al enviar datos por UDP: " << send_error.message() << "\n";
                return 1;
            }

        } catch (const std::exception& e) {
            std::cerr << "netcp: error: " << e.what() << "\n";
            return 1;
        }

        return EXIT_SUCCESS;
    }

    return 0;
}