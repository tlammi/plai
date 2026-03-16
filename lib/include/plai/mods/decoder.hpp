#pragma once

#include <plai/frac.hpp>
#include <plai/media2/media.hpp>
#include <plai/mods/module.hpp>
#include <stdexec/execution.hpp>

namespace plai::mods {

class DecodingStream {};

class Decoder : public Module {
 public:
    Decoder();

    void start(Context& ctx) override {}

    void finish() override {}
    void stop() override {}

    size_t enqueued() const;

    void enqueue(media2::Media input);

 private:
    void work();
    stdexec::run_loop m_loop;
    std::jthread m_worker;
};

std::unique_ptr<Decoder> make_decoder();

}  // namespace plai::mods
