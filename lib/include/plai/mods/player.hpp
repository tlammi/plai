#pragma once

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

    virtual ex::AnySenderOf<media2::Media> next_media() = 0;

 private:
};

class Player : public Module {
 public:
    explicit Player(MediaStream& stream) : m_stream(&stream) {}

    void start(Context& ctx) override {
        exec::start_detached(stdexec::on(
            ctx.scheduler(),
            m_stream->next_media() | stdexec::then([](auto media) {
                PLAI_DEBUG("received media with size: {}", media.data().size());
            })));
        exec::start_detached(stdexec::on(
            ctx.scheduler(),
            stdexec::just() | stdexec::then([&ctx]() { ctx.request_stop(); })));
    }

    void stop() override {}

 private:
    MediaStream* m_stream;
};
}  // namespace plai::mods
