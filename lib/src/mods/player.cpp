#include <plai/media/frame_converter.hpp>
#include <plai/mods/player.hpp>
#include <plai/sched/task.hpp>
#include <plai/thirdparty/magic_enum.hpp>
#include <plai/util/match.hpp>
#include <plai/util/memfn.hpp>

#include "alpha_calc.hpp"

namespace plai::mods {
using namespace std::literals;
namespace {

constexpr auto MAIN_TARGET =
    RenderTarget{.vertical = Position::Middle, .horizontal = Position::Middle};

constexpr auto WATERMARK_BLEND = 500ms;
constexpr Duration PERIOD_30MS = 33'333us;
constexpr Duration PERIOD_60MS = 16'667us;
constexpr auto MAX_ALPHA = std::numeric_limits<uint8_t>::max();

enum class State {
    Init,
    Vid,
    Img,
    ImgDelay,
};

enum class BlendState {
    None,
    Prepare,
    FadeInWatermarks,
    Blend,
    FadeOutWatermarks,
};

constexpr Duration meta_to_period(const DecodingMeta& meta) {
    if (meta.still()) return 100ms;
    static constexpr auto micro = 1'000'000;
    auto uspf = meta.fps.reciprocal() * micro;
    auto uspf_int =
        static_cast<uint64_t>(uspf.num()) / static_cast<uint64_t>(uspf.den());
    auto dur = std::chrono::microseconds(uspf_int);
    return dur;
}

}  // namespace

class PlayerImpl final : public Player {
    using Sink<Decoded>::notify_sink_ready;

 public:
    PlayerImpl(sched::Executor exec, Frontend* frontend, play::PlayerOpts opts)
        : m_exec(std::move(exec)), m_front(frontend), m_opts(std::move(opts)) {
        m_watermark_textures.reserve(m_opts.watermarks.size());
        for (size_t i = 0; i < m_opts.watermarks.size(); ++i) {
            m_watermark_textures.push_back(m_front->texture());
            m_watermark_textures.back()->update(m_opts.watermarks.at(i).image);
        }
    }

    void consume(Decoded decoded) override {
        auto lk = std::lock_guard(m_mut);
        m_buf = std::move(decoded);
    }

    bool sink_ready() override {
        auto lk = std::lock_guard(m_mut);
        return !m_buf;
    }

 private:
    void consume_events() {
        while (true) {
            auto evt = m_front->poll_event();
            if (!evt) break;
            match(*evt, [](Quit /*quit*/) {
                PLAI_ERR("Exiting via frontend not implemented :(");
            });
        }
    }

    void handle_frame(media::Frame frm) {
        auto dims = frm.dims();
        dims.scale_to(m_dims);
        frm = m_conv(dims, frm);
        m_text->update(frm);
    }

    auto extract_buf() {
        auto lk = std::unique_lock(m_mut);
        auto item = std::exchange(m_buf, std::nullopt);
        lk.unlock();
        if (!item) return item;
        notify_sink_ready();
        auto* frm = std::get_if<media::Frame>(&*item);
        if (frm) {
            auto dims = frm->dims();
            dims.scale_to(m_dims);
            *frm = m_conv(dims, *frm);
        }
        return item;
    }

    void step_init() {
        auto item = extract_buf();
        if (!item) return;
        match(
            *std::move(item),
            [&](DecodingMeta meta) {
                m_render_task.set_period(meta_to_period(meta));
                if (meta.still())
                    m_st = State::Img;
                else
                    m_st = State::Vid;
            },
            [&](media::Frame frm) {
                PLAI_WARN(
                    "Got frame input while in Init state. Deducing as video");
                m_st = State::Vid;
            });
    }

    void step_img() {
        auto item = extract_buf();
        if (!item) return;
        match(
            *std::move(item),
            [&](DecodingMeta meta) {
                m_render_task.set_period(PERIOD_30MS);
                m_prev_st =
                    std::exchange(m_st, meta.still() ? State::Img : State::Vid);
                m_blend_state = BlendState::Prepare;
                m_next_meta = meta;
                m_alpha_calc = AlphaCalc(m_opts.blend_dur);
            },
            [&](media::Frame frm) {
                m_prev_frm = std::exchange(m_frm, std::move(frm));
                m_text->update(m_frm);
                m_text->render_to(MAIN_TARGET);
                m_timestamp = Clock::now();
                m_st = State::ImgDelay;
            });
    }

    void step_img_delay() {
        auto now = Clock::now();
        if (now - m_timestamp >= m_opts.image_dur) m_st = State::Img;
        m_text->render_to(MAIN_TARGET);
    }

    void step_vid() {
        auto item = extract_buf();
        if (!item) {
            PLAI_WARN("No video frame available!");
            return;
        }
        match(
            *std::move(item),
            [&](DecodingMeta meta) {
                m_render_task.set_period(PERIOD_30MS);
                m_prev_st =
                    std::exchange(m_st, meta.still() ? State::Img : State::Vid);
                m_next_meta = meta;
            },
            [&](media::Frame frm) {
                m_prev_frm = std::exchange(m_frm, std::move(frm));
                m_text->update(m_frm);
                m_text->render_to(MAIN_TARGET);
            });
    }

    void step_blend_prepare() {
        auto item = extract_buf();
        if (!item) {
            PLAI_WARN("No image frame available for blending");
            return;
        }
        match(
            *(std::move(item)),
            [&](DecodingMeta meta) {
                return;
                PLAI_ERR("Got consecutive media metas");
            },
            [&](media::Frame frm) {
                m_prev_frm = std::exchange(m_frm, std::move(frm));
                m_alpha_calc = AlphaCalc(m_opts.blend_dur);
                using enum State;
                if (m_st == Vid && m_prev_st == Vid) {
                    m_blend_state = BlendState::Blend;
                } else if (m_st == Img && m_prev_st == Vid) {
                    m_alpha_calc = AlphaCalc(WATERMARK_BLEND);
                    m_blend_state = BlendState::FadeInWatermarks;
                } else if (m_st == Vid && m_prev_st == Img) {
                    m_blend_state = BlendState::Blend;
                } else if (m_st == Img && m_prev_st == Img) {
                    m_blend_state = BlendState::Blend;
                } else {
                    PLAI_FATAL(
                        "Invalid combination of states while preparing to "
                        "blend: {} and {}",
                        underlying_cast(m_st), underlying_cast(m_prev_st));
                    std::terminate();
                }
            });
    }

    void step_blend_fade_in_watermarks() {
        auto alpha = m_alpha_calc();
        m_front->render_clear();
        m_back->update(m_prev_frm);
        render_watermarks(alpha);
        if (alpha == MAX_ALPHA) {
            m_alpha_calc = AlphaCalc(m_opts.blend_dur);
            m_blend_state = BlendState::Blend;
        }
    }
    void step_blend_main_frames() {
        auto alpha = m_alpha_calc();
        m_back->update(m_prev_frm);
        m_back->alpha(MAX_ALPHA - alpha);
        m_back->render_to(MAIN_TARGET);
        m_text->update(m_frm);
        m_text->alpha(alpha);
        m_text->render_to(MAIN_TARGET);
        if (alpha == MAX_ALPHA) {
            if (m_st == State::Vid && m_prev_st == State::Img) {
                m_alpha_calc = AlphaCalc(WATERMARK_BLEND);
                m_blend_state = BlendState::FadeOutWatermarks;
            } else {
                m_blend_state = BlendState::None;
            }
        }
    }

    void step_blend_fade_out_watermarks() {
        auto alpha = m_alpha_calc();
        render_watermarks(MAX_ALPHA - alpha);
        if (alpha == MAX_ALPHA) { m_blend_state = BlendState::None; }
    }

    void step_normal() {
        switch (m_st) {
            case State::Init: step_init(); return;
            case State::Vid: step_vid(); return;
            case State::Img: step_img(); return;
            case State::ImgDelay: step_img_delay(); return;
        }
    }

    void step() {
        PLAI_TRACE("Stepping player. State: {}", magic_enum::enum_name(m_st));
        std::println("Stepping player. State: {}", magic_enum::enum_name(m_st));
        using enum BlendState;
        switch (m_blend_state) {
            case None: step_normal(); break;
            case Prepare: step_blend_prepare(); break;
            case FadeInWatermarks: step_blend_fade_in_watermarks(); break;
            case Blend: step_blend_main_frames(); break;
            case FadeOutWatermarks: step_blend_fade_out_watermarks(); break;
        }
        m_front->render_current();
    }

    void render_watermarks(uint8_t alpha = MAX_ALPHA) {
        for (size_t i = 0; i < m_watermark_textures.size(); ++i) {
            auto& text = m_watermark_textures.at(i);
            auto& watermark = m_opts.watermarks.at(i);
            text->blend_mode(BlendMode::Blend);
            text->alpha(alpha);
            text->render_to(watermark.target);
        }
    }

    std::mutex m_mut{};
    std::optional<Decoded> m_buf{};
    Vec<int> m_dims{1920, 1080};
    media::FrameConverter m_conv{};
    State m_st{State::Init};
    State m_prev_st{State::Init};
    BlendState m_blend_state{BlendState::None};
    TimePoint
        m_timestamp{};  // Used for operations that take multiple iterations
    AlphaCalc m_alpha_calc{};
    DecodingMeta
        m_next_meta{};  // Need to read the first frame after this for blending
    media::Frame m_frm{};
    media::Frame m_prev_frm{};

    sched::Executor m_exec;
    Frontend* m_front;
    play::PlayerOpts m_opts;
    std::vector<std::unique_ptr<Texture>> m_watermark_textures{};
    std::unique_ptr<Texture> m_watermark_text{m_front->texture()};
    std::unique_ptr<Texture> m_text{m_front->texture()};
    std::unique_ptr<Texture> m_back{m_front->texture()};

    sched::PeriodicTask m_event_consumer =
        sched::task() | sched::period(100ms) |
        memfn(this, &PlayerImpl::consume_events) | sched::executor(m_exec) |
        sched::task_finish();

    sched::PeriodicTask m_render_task =
        sched::task() | sched::period(500ms) | sched::executor(m_exec) |
        memfn(this, &PlayerImpl::step) | sched::task_finish();
};
std::unique_ptr<Player> make_player(sched::Executor exec, Frontend* frontend,
                                    play::PlayerOpts opts) {
    return std::make_unique<PlayerImpl>(std::move(exec), frontend,
                                        std::move(opts));
}

}  // namespace plai::mods
