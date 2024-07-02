#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <print>

int main(int argc, char** argv) {
  try {
    if (argc != 2)
      throw std::runtime_error(std::format("usage: {} path/to/file", argv[0]));

    plai::media::Demux demux{argv[1]};
    plai::media::Packet pkt{};
    auto [stream_idx, stream] = demux.streams().any_video_stream();
    plai::media::Decoder decoder{stream};
    plai::media::Frame frm{};
    size_t index = 0;
    while (demux >> pkt) {
      if (pkt.stream_index() != stream_idx) continue;
      decoder << pkt;
      if (!(decoder >> frm)) continue;

      ++index;
      std::println(stderr, "decoded frame no {}", index);
    }

  } catch (std::exception& e) {
    std::println(stderr, "ERROR: {}", e.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

