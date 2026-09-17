#pragma once
#include <cstdint>
#include <string>

class Arguments {
  public:
    const std::uint16_t DEFAULT_PORT = 389;
    Arguments(int argc, char **argv);
    std::uint16_t port;
    std::string file_path;
};