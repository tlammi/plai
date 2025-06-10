#include <plai/fs/read.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>
#include <plai/play/player.hpp>
#include <plai/util/defer.hpp>
#include <plai/util/str.hpp>
#include <print>

int main(int argc, char** argv) {
    if (argc != 2)
        throw std::runtime_error(
            std::format("usage: {} path/to/file", argv[0]));
    auto demux = plai::media::Demux();
    auto media = plai::fs::read_bin(argv[1]);
    demux = plai::media::Demux(media);
    auto [stream_idx, stream] = demux.best_video_stream();
    auto still = stream.is_still_image();
    auto decoder = plai::media::Decoder(stream);
    if (still) {
        auto pkt = plai::media::Packet();
        auto frm = plai::media::Frame();
        auto real_frm = plai::media::Frame();
        while (demux >> pkt) {
            if (pkt.stream_index() != stream_idx) continue;
            decoder << pkt;
            if (!(decoder >> frm)) continue;
            if (frm.width() > real_frm.width())
                real_frm = std::exchange(frm, {});
        }
    } else {
        std::println("decoding frame from video");
        auto pkt = plai::media::Packet();
        auto frm = plai::media::Frame();
        while (demux >> pkt) {
            std::println("extracted pkt");
            if (pkt.stream_index() != stream_idx) continue;
            decoder << pkt;
            std::println("pkt passed to decoder");
            if (!(decoder >> frm)) continue;
            std::println("extracted frame");
        }
    }
}
