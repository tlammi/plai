#pragma once

#include <plai/media2/media.hpp>
#include <plai/mods/module.hpp>

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

    void start(Context& ctx) override {}

    void stop() override {}

 private:
    MediaStream* m_stream;
};
}  // namespace plai::mods
