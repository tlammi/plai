#include <plai/exceptions.hpp>
#include <plai/fs/read.hpp>
#include <plai/media/util.hpp>

namespace plai::media {

Frame decode_image(std::span<const uint8_t> data) {
    auto demux = Demux(data);
    auto [stream_idx, stream] = demux.best_video_stream();
    auto decoder = Decoder(stream);
    auto pkt = Packet();
    auto frm = Frame();
    while (demux >> pkt) {
        decoder << pkt;
        if (!(decoder >> frm)) continue;
        return frm;
    }
    throw ValueError("failed to decode image");
}
Frame decode_image(const std::filesystem::path& path) {
    return decode_image(fs::read_bin(path));
}
}  // namespace plai::media
