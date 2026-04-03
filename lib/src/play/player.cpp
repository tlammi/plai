#include <plai/logs/logs.hpp>
#include <plai/play/player.hpp>
#include <plai/util/match.hpp>
#include <variant>

#include "alpha_calc.hpp"
#include "media_processor.hpp"

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
class Player::Impl final : MediaProcessor::Input, MediaProcessor::Output {
 public:
    Impl(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
        : m_front(front),
          m_src(media_src),
          m_opts(std::move(opts)),
          m_processor(
              *this, *this,
              MediaProcessor::Opts{.hwaccel = make_hwaccel(m_opts.accel)}) {
        m_watermark_textures.reserve(m_opts.watermarks.size());
        for (size_t i = 0; i < m_opts.watermarks.size(); ++i) {
            m_watermark_textures.push_back(m_front->texture());
            m_watermark_textures.back()->update(m_opts.watermarks.at(i).image);
        }
        // TODO: inspect why this is needed, without this decoding largish JPEG
        // images may (and does so maybe 9/10 times) crash the program.
        m_processor.set_dims({1920, 1080});
    }

    void run() {
        try {
            while (true) {
                poll_front();
                {
                    auto media_lock = std::unique_lock(m_media_mut);
                    if (!m_enqueued_media) {
                        auto next = m_src->next_media();
                        if (!next) {
                            if (!m_opts.wait_media) { m_exiting = true; }
                        } else {
                            m_enqueued_media = *std::move(next);
                            media_lock.unlock();
                            m_media_cv.notify_one();
                        }
                    }
                }
                // consume_next() goes to various callbacks inherited from
                // MediaProcessor::Output
                auto consumed = m_processor.consume_next();
                if (!consumed) {
                    if (m_exiting) return;
                    std::this_thread::sleep_for(10ms);
                    continue;
                }
                m_rlimit();
            }
        } catch (const Cancelled&) {
            PLAI_TRACE("Cancellation caught");
            m_processor.stop();
            m_exiting = true;
            m_media_cv.notify_one();
        }
    }

    void stop() {
        PLAI_DEBUG("stopping player");
        m_front->stop();
    }

    void clear_media_queue() {
        PLAI_WARN("clear_media_queue has been deprecated");
    }

 private:
    // MediaProcessor::Input
    media::Media next_media() override {
        auto lk = std::unique_lock(m_media_mut);
        m_media_cv.wait(lk, [&] { return m_enqueued_media || m_exiting; });
        if (m_exiting) throw Cancelled();
        return std::exchange(m_enqueued_media, {});
    }

    // MediaProcessor::Output
    void new_media(media::Frame frm, bool still, Frac<int> fps) override {
        assert(frm && "empty frame received by new_media()");
        poll_front();
        m_frame_count = 1;
        const auto was_still = std::exchange(m_still, still);
        if (m_prev_frame) {
            do_blend(was_still, m_still, frm);
        } else {
            m_front_text->update(frm);
            m_front_text->render_to(IMG_TARGET);
        }
        std::swap(m_prev_frame, frm);
        render_watermarks(m_still ? std::numeric_limits<uint8_t>::max() : 0);
        m_front->render_current();
        m_rlimit = rate_limiter(fps);
    }

    // MediaProcessor::Output
    void new_frame(media::Frame frm) override {
        poll_front();
        m_front_text->update(frm);
        m_front_text->render_to(IMG_TARGET);
        ++m_frame_count;
        std::swap(m_prev_frame, frm);
        render_watermarks(m_still ? std::numeric_limits<uint8_t>::max() : 0);
        m_front->render_current();
    }

    // MediaProcessor::Output
    void media_end_reached() override {
        poll_front();
        if (m_frame_count == 1) {
            // Showed total of one frame -> the previous media was an image
            do_image_delay();
        }
        PLAI_DEBUG("showed media with {} frames", m_frame_count);
    }

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

    RateLimiter rate_limiter(Frac<int> fps) {
        if (m_opts.unlimited_fps || !fps.num())
            return RateLimiter(Duration::zero());
        static constexpr auto micro = 1'000'000;
        auto uspf = fps.reciprocal() * micro;
        auto uspf_int = static_cast<uint64_t>(uspf.num()) /
                        static_cast<uint64_t>(uspf.den());
        auto dur = std::chrono::microseconds(uspf_int);
        return RateLimiter(dur);
    }

    void do_blend(bool was_still, bool is_still, const media::Frame& frm) {
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
            m_back_text->alpha(std::numeric_limits<uint8_t>::max());
            poll_loop([&] {
                m_front->render_clear();
                m_back_text->update(m_prev_frame);
                auto alpha = watermark_alpha_calc();
                m_back_text->render_to(IMG_TARGET);
                render_watermarks(alpha);
                m_front->render_current();
                return alpha == max_alpha;
            });
        }
        const auto watermark_static_alpha =
            is_still || was_still ? max_alpha : 0;
        auto alpha_calc = AlphaCalc(m_opts.blend_dur);
        poll_loop([&] {
            m_front->render_clear();
            auto alpha = alpha_calc();
            m_back_text->alpha(max_alpha - alpha);
            m_back_text->update(m_prev_frame);
            m_back_text->render_to(IMG_TARGET);
            m_front_text->alpha(alpha);
            m_front_text->update(frm);
            m_front_text->render_to(IMG_TARGET);
            render_watermarks(watermark_static_alpha);
            m_front->render_current();
            return alpha == max_alpha;
        });

        if (was_still && !is_still) {
            auto watermark_alpha_calc = AlphaCalc(watermark_blend);
            poll_loop([&] {
                m_front->render_clear();
                m_front_text->update(frm);
                m_front_text->render_to(IMG_TARGET);
                auto alpha = watermark_alpha_calc();
                render_watermarks(max_alpha - alpha);
                m_front->render_current();
                return alpha == max_alpha;
            });
        }
    }

    void do_image_delay() {
        PLAI_TRACE("showing image");
        auto start = Clock::now();
        auto end = start + m_opts.image_dur;

        while (Clock::now() < end) {
            poll_front();
            std::this_thread::sleep_for(100ms);
        }
        PLAI_TRACE("image showed");
    }

    void poll_front() {
        while (true) {
            auto evt = m_front->poll_event();
            if (!evt) return;
            if (!m_exiting && std::holds_alternative<Quit>(*evt)) {
                PLAI_DEBUG("Received quit command from frontend");
                throw Cancelled();
            }
        }
    }

    template <class F>
    void poll_loop(F f) {
        while (true) {
            poll_front();
            if (f()) return;
        }
    }

    Frontend* m_front;
    MediaSrc* m_src;
    PlayerOpts m_opts;
    MediaProcessor m_processor;
    media::Frame m_prev_frame{};
    std::mutex m_media_mut{};
    std::condition_variable m_media_cv{};
    media::Media m_enqueued_media{};
    std::vector<std::unique_ptr<Texture>> m_watermark_textures{};
    std::unique_ptr<Texture> m_front_text{m_front->texture()};
    std::unique_ptr<Texture> m_back_text{m_front->texture()};
    RateLimiter m_rlimit{Duration::zero()};
    size_t m_frame_count{};
    bool m_exiting{false};
    bool m_still{false};
};

Player::Player(Frontend* front, MediaSrc* media_src, PlayerOpts opts)
    : m_impl(std::make_unique<Impl>(front, media_src, std::move(opts))) {}
Player::~Player() {}

void Player::run() { m_impl->run(); }
void Player::stop() { m_impl->stop(); }
void Player::clear_media_queue() { m_impl->clear_media_queue(); }

}  // namespace plai::play
