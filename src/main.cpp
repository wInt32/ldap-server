#include <cstdlib>

#include "args.hpp"
#include "server.hpp"

int main(int argc, char **argv) {
    Arguments args{argc, argv};

    LdapServer server{args.port, args.file_path};

    server.run();

    return EXIT_SUCCESS;
}