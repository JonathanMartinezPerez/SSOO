#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <system_error>
#include <unistd.h>
#include <errno.h>

namespace tools {
    struct program_options  {
        bool show_help = false;
        std::string output_filename;
        // ...
    };

    std::string getenv(const std::string& name){
        char* value = std::getenv(name.c_str());
        if (value) {
            return std::string(value);
        }
        else {
        return std::string();
        }
    }

    std::optional<program_options> parse_args(int argc, char* argv[]){
        std::vector<std::string_view> args(argv + 1, argv + argc);
        program_options options;

        for (auto it = args.begin(), end = args.end(); it != end; ++it){
            if (*it == "-h" || *it == "--help"){
                options.show_help = true;
            }
            if (*it == "-o" || *it == "--output"){
                if (++it != end){
                    options.output_filename = *it;
                }
                else{
                    std::cerr << "netcp: falta un archivo como argumento\nModo de empleo: netcp [-h] ORIGEN";
                    return std::nullopt;
                }
            }
        }
        return options;
    }

    void print_usage(){
        std::cout << "Usage: ./netcp -h -o <output_file>\n";
        std::cout << "-h: Muestra esta ayuda\n";
        std::cout << "-o: Archivo de salida\n";
    }

    std::error_code read_file(int fd, std::vector<uint8_t>& buffer){
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read < 0){
            return std::error_code(errno, std::system_category());
        }
        buffer.resize(bytes_read);
        return std::error_code(0, std::system_category());
    }

}