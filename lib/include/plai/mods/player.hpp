#pragma once

#include <plai/frontend/frontend.hpp>
#include <plai/mods/decoder.hpp>
#include <plai/play/player.hpp>

namespace plai::mods {

class Player : public flow::Sink<Decoded> {
 public:
};

std::unique_ptr<Player> make_player(sched::Executor exec, Frontend* frontend,
                                    play::PlayerOpts opts = {});

}  // namespace plai::mods
