#include <plai/logs/logs.hpp>
#include <plai/media/decoding_pipeline.hpp>
#include <plai/play/player.hpp>
#include <plai/util/match.hpp>
#include <variant>

#include "alpha_calc.hpp"

namespace plai::play {
namespace {
constexpr auto IMG_TARGET =
    RenderTarget{.vertical = Position::Middle, .horizontal = Position::Middle};
}
using namespace std::literals::chrono_literals;
namespace {
media::HwAccel make_hwaccel(const std::string& nm) {
    if (nm == "sw") {
        PLAI_DEBUG("Using a software accelerator");
        return {};
    }
    PLAI_DEBUG("looking up accelerator with name '{}'", nm);
    auto res = media::lookup_hardware_accelerator(nm.c_str());
    if (!res) {
        PLAI_FATAL("Could not find hardware accelerator with name '{}'",
                   nm.c_str());
        // This should have a better exception
        throw ValueError("asdf");
    }
    return res;
}
enum class State {
    EmptyDecoder,    // decoder has no data in it, initial state
    FullDecoder,     // decoder has data but not full
    PartialDecoder,  // decoder's buffer is full, don't push more stuff
    EndOfMedias,     // No more medias available and no wait configured
    Done,            // All done, about to return
};
}  // namespace
class Player::Impl {
 public:
    static constexpr auto MAX_QUEUED_MEDIAS = std::size_t(5);

    Impl(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
        : m_front(front),
          m_src(media_src),
          m_opts(std::move(opts)),
          m_decoder(make_hwaccel(m_opts.accel)) {
        m_watermark_textures.reserve(m_opts.watermarks.size());
        for (size_t i = 0; i < m_opts.watermarks.size(); ++i) {
            m_watermark_textures.push_back(m_front->texture());
            m_watermark_textures.back()->update(m_opts.watermarks.at(i).image);
        }
        // TODO: inspect why this is needed, without this decoding largish JPEG
        // images may (and does so maybe 9/10 times) crash the program.
        m_decoder.set_dims({1920, 1080});
    }

    void run() {
        auto rlimit = RateLimiter(Duration::zero());
        uint32_t frame_count = 0;
        while (true) {
            if (!poll_front()) return;
            if (m_clear_media_queue.exchange(false)) {
                m_decoder.clear_media_queue();
                m_queued_medias = 0;
            }
            if (!m_queued_medias && m_exiting) return;
            if (m_queued_medias < MAX_QUEUED_MEDIAS) {
                auto next = m_src->next_media();
                if (!next) {
                    if (!m_opts.wait_media) { m_exiting = true; }
                } else {
                    m_decoder.decode(
                        match(std::move(*next), [](auto v) { return v.data; }));
                    ++m_queued_medias;
                }
            }
            if (m_queued_medias) {
                if (!m_stream) {
                    m_stream = m_decoder.frame_stream();
                    m_stream_iter = m_stream->begin();
                    const auto was_still =
                        std::exchange(m_still, m_stream->still());
                    if (m_prev_frame) {
                        if (!do_blend(was_still, m_still)) return;
                    }
                    rlimit = rate_limiter();
                } else {
                    std::swap(m_prev_frame, *m_stream_iter);
                    ++m_stream_iter;
                }
                m_front->render_clear();
                if (m_stream_iter != m_stream->end()) {
                    auto& frm = *m_stream_iter;
                    m_front_text->update(frm);
                    m_front_text->render_to(IMG_TARGET);
                    ++frame_count;
                } else {
                    PLAI_DEBUG("showed media with {} frames", frame_count);
                    if (frame_count == 1) {
                        if (!do_image_delay()) return;
                    }
                    frame_count = 0;
                    --m_queued_medias;
                    m_stream.reset();
                    continue;
                }
                render_watermarks(m_still ? std::numeric_limits<uint8_t>::max()
                                          : 0);
                m_front->render_current();
                rlimit();
                continue;
            }
            std::this_thread::sleep_for(10ms);
        }
    }

    void stop() { m_front->stop(); }

    void clear_media_queue() { m_clear_media_queue = true; }

 private:
    void render_watermarks(
        uint8_t alpha = std::numeric_limits<uint8_t>::max()) {
        for (size_t i = 0; i < m_watermark_textures.size(); ++i) {
            auto& text = m_watermark_textures.at(i);
            auto& watermark = m_opts.watermarks.at(i);
            text->blend_mode(BlendMode::Blend);
            text->alpha(alpha);
            text->render_to(watermark.target);
        }
    }

    RateLimiter rate_limiter() {
        if (m_opts.unlimited_fps) return RateLimiter(Duration::zero());
        assert(m_stream);
        static constexpr auto micro = 1'000'000;
        auto uspf = m_stream->fps().reciprocal() * micro;
        auto uspf_int = static_cast<uint64_t>(uspf.num()) /
                        static_cast<uint64_t>(uspf.den());
        auto dur = std::chrono::microseconds(uspf_int);
        return RateLimiter(dur);
    }

    bool do_blend(bool was_still, bool is_still) {
        PLAI_TRACE("blending");
        static constexpr auto max_alpha = std::numeric_limits<uint8_t>::max();
        static constexpr auto watermark_blend = 500ms;
        auto defer = Defer([&] {
            m_front_text->blend_mode(BlendMode::None);
            m_back_text->blend_mode(BlendMode::None);
            PLAI_TRACE("blended");
        });
        m_front_text->blend_mode(BlendMode::Blend);
        m_back_text->blend_mode(BlendMode::Blend);
        if (!was_still && is_still) {
            auto watermark_alpha_calc = AlphaCalc(watermark_blend);
            auto res = poll_loop([&] {
                m_front->render_clear();
                m_back_text->update(m_prev_frame);
                auto alpha = watermark_alpha_calc();
                render_watermarks(alpha);
                m_front->render_current();
                return alpha == max_alpha;
            });
            if (!res) return false;
        }
        const auto watermark_static_alpha =
            is_still || was_still ? max_alpha : 0;
        auto alpha_calc = AlphaCalc(m_opts.blend_dur);
        auto res = poll_loop([&] {
            m_front->render_clear();
            auto alpha = alpha_calc();
            m_back_text->alpha(max_alpha - alpha);
            m_back_text->update(m_prev_frame);
            m_back_text->render_to(IMG_TARGET);
            m_front_text->alpha(alpha);
            m_front_text->update(*m_stream_iter);
            m_front_text->render_to(IMG_TARGET);
            render_watermarks(watermark_static_alpha);
            m_front->render_current();
            return alpha == max_alpha;
        });
        if (!res) return false;

        if (was_still && !is_still) {
            auto watermark_alpha_calc = AlphaCalc(watermark_blend);
            auto res = poll_loop([&] {
                m_front->render_clear();
                m_front_text->update(*m_stream_iter);
                m_front_text->render_to(IMG_TARGET);
                auto alpha = watermark_alpha_calc();
                render_watermarks(max_alpha - alpha);
                m_front->render_current();
                return alpha == max_alpha;
            });
            if (!res) return res;
        }
        return true;
    }

    bool do_image_delay() {
        PLAI_TRACE("showing image");
        auto start = Clock::now();
        auto end = start + m_opts.image_dur;

        while (Clock::now() < end) {
            if (!poll_front()) return false;
            std::this_thread::sleep_for(100ms);
        }
        PLAI_TRACE("image showed");
        return true;
    }

    bool poll_front() {
        while (true) {
            auto evt = m_front->poll_event();
            if (!evt) return true;
            if (std::holds_alternative<Quit>(*evt)) return false;
        }
    }

    template <class F>
    bool poll_loop(F f) {
        while (true) {
            if (!poll_front()) return false;
            if (f()) return true;
        }
    }

    Frontend* m_front;
    MediaSrc* m_src;
    PlayerOpts m_opts;
    media::DecodingPipeline m_decoder{};
    std::optional<media::DecodingStream> m_stream{};
    media::DecodingStream::Iter m_stream_iter{};
    media::Frame m_prev_frame{};
    std::vector<std::unique_ptr<Texture>> m_watermark_textures{};
    std::unique_ptr<Texture> m_front_text{m_front->texture()};
    std::unique_ptr<Texture> m_back_text{m_front->texture()};
    size_t m_queued_medias{0};
    bool m_exiting{false};
    bool m_still{false};
    std::atomic_bool m_clear_media_queue{false};
};

Player::Player(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
    : m_impl(std::make_unique<Impl>(front, media_src, std::move(opts))) {}
Player::~Player() {}

void Player::run() { m_impl->run(); }
void Player::stop() { m_impl->stop(); }
void Player::clear_media_queue() { m_impl->clear_media_queue(); }

}  // namespace plai::play
