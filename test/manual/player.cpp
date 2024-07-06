
#include <format>
#include <plai/frontend/frontend.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <print>

using plai::media::Decoder;
using plai::media::Demux;
using plai::media::Frame;
using plai::media::Packet;

int main(int argc, char** argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(std::format("usage: {} path/to/file", argv[0]));
    plai::logs::init(plai::logs::Level::Trace);
    auto front = plai::frontend("sdl2").commit();

    auto demux = Demux(argv[1]);
    auto [stream_idx, stream] = demux.streams().any_video_stream();
    auto decoder = Decoder(stream);
    Packet pkt{};
    Frame frm{};
    while (demux >> pkt) {
      if (pkt.stream_index() != stream_idx) continue;
      decoder << pkt;
      if (!(decoder >> frm)) continue;
      if (!frm.width()) break;
    }

  } catch (const std::exception& e) {
    std::println(stderr, "ERROR: {}", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
