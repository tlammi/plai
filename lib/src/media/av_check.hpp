#pragma once

#include <plai/media/exceptions.hpp>

namespace plai::media::detail {

#define AV_CHECK(expr)                                          \
  do {                                                          \
    int internal_av_check_res_ = (expr);                        \
    if (internal_av_check_res_) [[unlikely]]                    \
      throw ::plai::media::AVException(internal_av_check_res_); \
  } while (0)

}  // namespace plai::media::detail
