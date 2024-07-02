/**
 * \brief Calculae number of packets in a file
 * */

#include <plai/logs/logs.hpp>
#include <plai/media/demux.hpp>
#include <print>

int main(int argc, char** argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(std::format("usage: {} path/to/media", argv[0]));
    plai::logs::init(plai::logs::Level::Trace);
    auto m = plai::media::Demux(argv[1]);
    plai::media::Packet pkt{};
    size_t count = 0;
    while (m >> pkt) ++count;
    std::println("{} packets", count);
  } catch (const std::exception& e) {
    std::println(stderr, "ERROR: {}", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

