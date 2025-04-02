#pragma once

#include <malloc.h>
namespace plai {

inline size_t allocated_memory() noexcept { return mallinfo2().arena; }

}  // namespace plai
