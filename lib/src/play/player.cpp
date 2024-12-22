#include <plai/media/decoding_pipeline.hpp>
#include <plai/play/player.hpp>
#include <print>
#include <variant>

namespace plai::play {
using namespace std::literals::chrono_literals;
namespace {
enum class State {
    EmptyDecoder,    // decoder has no data in it, initial state
    FullDecoder,     // decoder has data but not full
    PartialDecoder,  // decoder's buffer is full, don't push more stuff
    EndOfMedias,     // No more medias available and no wait configured
    Done,            // All done, about to return
};
}
class Player::Impl {
 public:
    static constexpr auto MAX_QUEUED_MEDIAS = std::size_t(5);

    Impl(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
        : m_front(front), m_src(media_src), m_opts(std::move(opts)) {
        m_watermark_textures.reserve(m_opts.watermarks.size());
        for (size_t i = 0; i < m_opts.watermarks.size(); ++i) {
            m_watermark_textures.push_back(m_front->texture());
            m_watermark_textures.back()->update(m_opts.watermarks.at(i).image);
        }
    }

    void run() {
        auto rlimit = RateLimiter(Duration::zero());
        while (true) {
            while (true) {
                auto event = m_front->poll_event();
                if (!event) break;
                if (std::holds_alternative<Quit>(*event)) return;
            }
            if (!m_queued_medias && m_exiting) return;
            if (m_queued_medias < MAX_QUEUED_MEDIAS) {
                auto next = m_src->next_media();
                if (!next) {
                    if (!m_opts.wait_media) { m_exiting = true; }
                } else {
                    m_decoder.decode(std::move(*next));
                    ++m_queued_medias;
                }
            }
            if (m_queued_medias) {
                if (!m_stream) {
                    m_stream = m_decoder.frame_stream();
                    m_stream_iter = m_stream->begin();
                    rlimit = rate_limiter();
                } else {
                    ++m_stream_iter;
                }
                m_front->render_clear();
                if (m_stream_iter != m_stream->end()) {
                    m_front_text->update(*m_stream_iter);
                    m_front_text->render_to({});
                } else {
                    --m_queued_medias;
                    m_stream.reset();
                    continue;
                }
                render_watermarks();
                m_front->render_current();
                rlimit();
                continue;
            }
            std::this_thread::sleep_for(1ms);
        }
    }

 private:
    void render_watermarks() {
        for (auto elems :
             std::ranges::zip_view(m_watermark_textures, m_opts.watermarks)) {
            auto& [text, watermark] = elems;
            text->render_to(watermark.target);
        }
    }

    RateLimiter rate_limiter() {
        std::println("rate_limiter");
        if (m_opts.unlimited_fps) return RateLimiter(Duration::zero());
        assert(m_stream);
        static constexpr auto micro = 1'000'000;
        auto uspf = m_stream->fps().reciprocal() * micro;
        auto uspf_int = static_cast<uint64_t>(uspf.num()) /
                        static_cast<uint64_t>(uspf.den());
        auto dur = std::chrono::microseconds(uspf_int);
        return RateLimiter(dur);
    }

    Frontend* m_front;
    MediaSrc* m_src;
    PlayerOpts m_opts;
    media::DecodingPipeline m_decoder{};
    std::optional<media::DecodingStream> m_stream{};
    media::DecodingStream::Iter m_stream_iter{};
    std::vector<std::unique_ptr<Texture>> m_watermark_textures{};
    std::unique_ptr<Texture> m_front_text{m_front->texture()};
    size_t m_queued_medias{0};
    bool m_exiting{false};
};

Player::Player(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
    : m_impl(std::make_unique<Impl>(front, media_src, std::move(opts))) {}
Player::~Player() {}

void Player::run() { m_impl->run(); }

}  // namespace plai::play
