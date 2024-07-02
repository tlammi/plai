#pragma once

#include <type_traits>

namespace plai::concepts {

template <class T>
concept enum_type = std::is_enum_v<T>;

}
