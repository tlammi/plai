#include <plai/media/frame_converter.hpp>
#include <plai/mods/player.hpp>
#include <plai/sched/task.hpp>
#include <plai/thirdparty/magic_enum.hpp>
#include <plai/util/match.hpp>
#include <plai/util/memfn.hpp>

#include "alpha_calc.hpp"
#include "mods/player/root_sm.hpp"

namespace plai::mods {
using namespace std::literals;

class PlayerImpl final : public Player {
    using Sink<Decoded>::notify_sink_ready;

 public:
    PlayerImpl(sched::Executor exec, Frontend* frontend, play::PlayerOpts opts)
        : m_exec(std::move(exec)),
          m_front(frontend),
          m_ctx{
              .opts = std::move(opts),
              .text = frontend->texture(),
              .back = frontend->texture(),
              .task = &m_render_task,
              .notify_sink_ready = [&] { notify_sink_ready(); },

          } {
        m_ctx.watermark_textures.reserve(m_ctx.opts.watermarks.size());
        for (size_t i = 0; i < m_ctx.opts.watermarks.size(); ++i) {
            m_ctx.watermark_textures.push_back(m_front->texture());
            m_ctx.watermark_textures.back()->update(
                m_ctx.opts.watermarks.at(i).image);
        }
    }

    void consume(Decoded decoded) override {
        auto lk = std::lock_guard(m_ctx.mut);
        m_ctx.buf = std::move(decoded);
    }

    bool sink_ready() override {
        auto lk = std::lock_guard(m_ctx.mut);
        return !m_ctx.buf;
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

    void step() {
        m_front->render_clear();
        m_sm();
        m_ctx.back->render_to(player::MAIN_TARGET);
        m_ctx.text->render_to(player::MAIN_TARGET);
        for (size_t i = 0; i < m_ctx.watermark_textures.size(); ++i) {
            auto& text = m_ctx.watermark_textures.at(i);
            auto& watermark = m_ctx.opts.watermarks.at(i);
            text->render_to(watermark.target);
        }
        m_front->render_current();
    }

    sched::Executor m_exec;
    Frontend* m_front;

    sched::PeriodicTask m_event_consumer =
        sched::task() | sched::period(100ms) |
        memfn(this, &PlayerImpl::consume_events) | sched::executor(m_exec) |
        sched::task_finish();

    sched::PeriodicTask m_render_task =
        sched::task() | sched::period(500ms) | sched::executor(m_exec) |
        memfn(this, &PlayerImpl::step) | sched::task_finish();

    player::Ctx m_ctx;
    st::StateMachine<player::RootSm> m_sm{std::in_place, m_ctx};
};
std::unique_ptr<Player> make_player(sched::Executor exec, Frontend* frontend,
                                    play::PlayerOpts opts) {
    return std::make_unique<PlayerImpl>(std::move(exec), frontend,
                                        std::move(opts));
}

}  // namespace plai::mods
