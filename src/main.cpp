#include "hw_peripherals.hpp"
#include "server.hpp"

#include <cstdint>
#include <iostream>
#include <unistd.h>

constexpr std::string_view data_folder = "/home/pi/data/";

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << '\n';

    return 1;
  }

  const uint16_t port{static_cast<uint16_t>(atoi(argv[1]))};

  init_system();

  while (true) {
    Server server(port, data_folder, T2_DEFAULT);
    server.Run();

    usleep(1000000);
  }

  return 0;
}
