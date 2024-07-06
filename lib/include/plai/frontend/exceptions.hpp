#pragma once
#include <plai/exceptions.hpp>

namespace plai {

class FrontendException : public Exception {
 public:
  template <class T>
  explicit FrontendException(T&& t) noexcept : Exception(std::forward<T>(t)) {}

 private:
};

}  // namespace plai
