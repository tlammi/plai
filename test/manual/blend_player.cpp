
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

Frame load_first_frame(Demux& demux, Decoder& decoder, auto stream_idx) {
    Packet pkt{};
    Frame frm{};
    while (demux >> pkt) {
        if (pkt.stream_index() != stream_idx) continue;
        decoder << pkt;
        if ((decoder >> frm)) break;
    }
    return frm;
}

int main(int argc, char** argv) {
    try {
        plai::logs::init(plai::logs::Level::Trace);
        auto front = plai::frontend("sdl2");
        std::unique_ptr<plai::Texture> prev_text{};
        auto text = front->texture();

        auto inputs = std::span<const char* const>(&argv[1], argc - 1);

        for (auto input : inputs) {
            auto demux = Demux(input);
            auto [stream_idx, stream] = demux.best_video_stream();
            auto decoder = Decoder(stream);
            Packet pkt{};
            auto frm = load_first_frame(demux, decoder, stream_idx);
            if (!frm.width()) break;
            if (prev_text) {
                uint8_t alpha = 0;
                text->update(frm);
                while (true) {
                    prev_text->alpha(0xff - alpha);
                    prev_text->render_to({});
                    text->alpha(alpha);
                    text->render_to({});
                    front->render_current();
                    if (alpha == 0xff) break;
                    ++alpha;
                }
            }
            while (demux >> pkt) {
                if (pkt.stream_index() != stream_idx) continue;
                decoder << pkt;
                if (!(decoder >> frm)) continue;
                if (!frm.width()) break;
                text->update(frm);
                text->render_to(plai::RenderTarget{});
                front->render_current();
            }
            prev_text = std::exchange(text, front->texture());
        }

    } catch (const std::exception& e) {
        std::println(stderr, "ERROR: {}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
