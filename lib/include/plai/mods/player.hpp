#pragma once

#include <exec/env.hpp>
#include <exec/repeat_until.hpp>
#include <exec/start_detached.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media2/media.hpp>
#include <plai/mods/module.hpp>
#include <print>

namespace plai::mods {

/**
 * \brief Stream of media consumed by a Player
 * */
class MediaStream {
 public:
    virtual ~MediaStream() = default;

    /**
     * \brief Next media to play
     *
     * Empty media indicates an empty media
     * */
    virtual media2::Media next_media() = 0;

 private:
};

class Player : public Module {
 public:
    enum class State {
        Idle,
        Run,
        Finish,
        Stop,
    };
    explicit Player(MediaStream& stream) : m_stream(&stream) {}

    void start(Context& ctx) override {
        m_st = State::Run;
        m_ctx = &ctx;
        play();
    }

    void play() {
        // Either already running or exiting
        if (m_st != State::Idle) return;
        detach([this] {
            if (!m_enqueued_media) m_enqueued_media = m_stream->next_media();
            run();
        });
    }

    void pause() {
        detach([this] { m_st = State::Idle; });
    }

    void finish() override {
        detach([this] { m_st = State::Finish; });
    }
    void stop() override {
        detach([this] { m_st = State::Stop; });
    }

 private:
    void run() {
        m_curr_media = std::exchange(m_enqueued_media, m_stream->next_media());
    }

    template <class Fn>
    void detach(Fn&& fn) {
        exec::start_detached(stdexec::schedule(m_ctx->scheduler()) |
                             stdexec::then(std::forward<Fn>(fn)));
    }

    media2::Media m_curr_media{};
    media2::Media m_enqueued_media{};
    MediaStream* m_stream;
    Context* m_ctx{};
    State m_st{State::Run};
};
}  // namespace plai::mods
