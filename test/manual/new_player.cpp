#include <plai/flow/spec.hpp>
#include <plai/frontend/frontend.hpp>
#include <plai/fs/read.hpp>
#include <plai/logs/logs.hpp>
#include <plai/mods/decoder.hpp>
#include <plai/mods/player.hpp>

struct MediaSrc final : public plai::flow::Src<plai::media::Media> {
    std::vector<std::vector<uint8_t>> medias{};
    size_t idx = 0;

    MediaSrc(std::vector<std::vector<uint8_t>> medias) noexcept
        : medias(std::move(medias)) {}

    plai::media::Media produce() override {
        idx %= medias.size();
        auto item = medias.at(idx);
        idx = (idx + 1) % medias.size();
        return plai::media::Image{item};
    }
    bool src_ready() override { return true; }
};

auto read_medias(int argc, char** argv) {
    auto out = std::vector<std::vector<uint8_t>>();
    for (size_t i = 1; i < argc; ++i) {
        out.push_back(plai::fs::read_bin(argv[i]));
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) std::terminate();
    plai::logs::init(plai::logs::Level::Debug);
    auto msrc = MediaSrc(read_medias(argc, argv));
    auto ctx = plai::sched::IoContext();
    auto decoding_ctx = boost::asio::thread_pool(1);
    auto decoder = plai::mods::make_decoder(decoding_ctx.get_executor());

    auto frontend = plai::frontend(plai::FrontendType::Sdl2);
    auto player = plai::mods::make_player(ctx.get_executor(), frontend.get());

    auto pipeline = plai::flow::pipeline(ctx.get_executor()) | msrc | *decoder |
                    *player | plai::flow::pipeline_finish();
    ctx.run();
}
