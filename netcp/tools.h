#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <system_error>
#include <unistd.h>
#include <errno.h>
#include <atomic>
#include <csignal>

//Namespace para las utilities
namespace tools {

    struct program_options {
        bool show_help = false;
        bool mode_listen = false;
        bool mode_send = false;
        bool mode_command = false;
        std::vector<std::string> args = {};
        std::string filename;
    };

    std::optional<program_options> parse_args(int argc, char *argv[]) {
        program_options options;
        int opt;

        while ((opt = getopt(argc, argv, "hl")) != -1) {
            switch (opt) {
                case 'h':
                    options.show_help = true;
                    break;
                case 'l':
                    options.mode_listen = true;
                    break;
                default:
                    std::cerr << "Error: Opción desconocida\n";
            }
        }

        // Verificar si se proporciona un argumento
        if (optind < argc) {
            options.filename = argv[optind];
        }

        options.mode_send = !options.mode_listen;

        return options;
    }

    //Función para mostrar la ayuda
    void print_usage() {
        std::cout << "Usage:\n";
        std::cout << "./netcp -h: Muestra esta ayuda\n";
        std::cout << "./netcp -l: Escucha el puerto\n";
        std::cout << "./netcp <Archivo de entrada>\n";
    }

    //Función para leer el fichero
    std::error_code read_file(int fd, std::vector<uint8_t> &buffer) {
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read < 0) {
            return std::make_error_code(static_cast<std::errc>(errno));
        }
        buffer.resize(bytes_read);
        return std::error_code(0, std::system_category());
    }

    // Dentro del namespace tools
    std::error_code write_file(int fd, std::vector<unsigned char>& buffer) {
        ssize_t bytes_written = write(fd, buffer.data(), buffer.size());

        if (bytes_written < 0) {
            return std::make_error_code(static_cast<std::errc>(errno));
        }
        buffer.resize(bytes_written);
        return std::error_code(0, std::system_category()); // Éxito
    }

    // Función auxiliar para manejar señales
    volatile std::atomic<bool> quit_requested(false);

    void signal_handler(int signal) {
        std::cerr << "netcp: terminando... (signal " << signal << ")\n";
        quit_requested.store(true);
    }

}
