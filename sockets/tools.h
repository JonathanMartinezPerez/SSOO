#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <system_error>
#include <unistd.h>
#include <errno.h>

//Namespace para las utilities
namespace tools {

struct program_options {
    bool show_help = false;
    std::string filename;
};

//Función para obtener el valor de una variable de entorno (parte2)
std::string getenv(const std::string &name) {
    char *value = std::getenv(name.c_str());
    if (value) {
        return std::string(value);
    } else {
        return std::string();
    }
}


//Función para procesar los argumentos de la línea de comandos, la -o tiene un arg detras que es el fichero
std::optional<program_options> parse_args(int argc, char *argv[]) {
    
    program_options options;
    int opt;
    while ((opt = getopt(argc, argv, "o:h")) != -1) { // -o file -h
        switch (opt) {
        case 'h':
            options.show_help = true;
            break;
        case 'o':
            options.filename = optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -h -o file\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return options;
}

//Función para mostrar la ayuda
void print_usage() {
    std::cout << "Usage: ./netcp -h -o <input_file>\n";
    std::cout << "-h: Muestra esta ayuda\n";
    std::cout << "-o: Archivo de entrada\n";
}

//Función para leer el fichero
std::error_code read_file(int fd, std::vector<uint8_t> &buffer) {
    ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
    if (bytes_read < 0) {
        return std::error_code(errno, std::system_category());
    }
    buffer.resize(bytes_read);
    return std::error_code(0, std::system_category());
}

}
