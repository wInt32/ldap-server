#include <cstring>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <limits>

#include "args.hpp"

static const char *USAGE = "Usage:\n   isa-ldapserver {-p <port>} -f <file>";

static void print_usage(void) {
    std::cout << USAGE << std::endl;
    std::exit(1);
}

static bool is_valid_tcp_port(int port) {
    return port > 0 && port < std::numeric_limits<std::uint16_t>::max();
}

Arguments::Arguments(int argc, char **argv) {
    this->port = Arguments::DEFAULT_PORT;
    std::string file_path = "";

    bool port_arg = false;
    bool file_arg = false;

    for (int i = 1; i < argc; i++) {
        if (port_arg) {
            port_arg = false;
            int port = std::atoi(argv[i]);
            if (!is_valid_tcp_port(port)) {
                print_usage();
            }
            this->port = port;
        } else if (file_arg) {
            file_arg = false;
            file_path = std::string(argv[i]);
        } else if (std::strcmp(argv[i], "-p") == 0) {
            port_arg = true;
        } else if (std::strcmp(argv[i], "-f") == 0) {
            file_arg = true;
        } else {
            print_usage();
        }
    }

    if (port_arg || file_arg) print_usage();
    if (file_path.empty()) print_usage();

    this->file_path = file_path;
    
    if (std::ifstream{file_path}.bad()) print_usage();
}