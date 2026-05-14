#pragma once

#include <plai/net/api.hpp>
#include <plai/play/media_src.hpp>
namespace plai::ctx {

/**
 * \brief Type for storing the program state
 * */
class Ctx : public net::ApiV2, public play::MediaSrc {};

std::unique_ptr<Ctx> make_context();
}  // namespace plai::ctx
