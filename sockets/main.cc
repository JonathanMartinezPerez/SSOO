#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <string_view>
#include <optional>
#include "utils.h"

int main(int argc, char* argv[]){
        auto options = tools::parse_args(argc, argv);
        if (! options){
            return EXIT_FAILURE;
        }
        // Usar options.value() para acceder a las opciones...
        if (options.value().show_help){
            tools::print_usage();
        }
        // ...
        return EXIT_SUCCESS;
}
