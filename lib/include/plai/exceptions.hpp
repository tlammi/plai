#pragma once

#include <stdexcept>

namespace plai {

class Exception : public std::runtime_error {
 protected:
    Exception() noexcept : std::runtime_error("") {}
    template <class T>
    Exception(T&& t) noexcept : std::runtime_error(std::forward<T>(t)) {}

 private:
};

class ValueError : public Exception {
 public:
    ValueError(const char* msg) noexcept : Exception(msg) {}
    ValueError(const std::string& s) noexcept : Exception(s) {}

 private:
};

}  // namespace plai
