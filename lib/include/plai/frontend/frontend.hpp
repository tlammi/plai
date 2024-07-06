#pragma once

#include <plai/frontend/type.hpp>
#include <plai/frontend/window.hpp>
#include <string_view>

namespace plai {

class Frontend {
 public:
  explicit Frontend(std::unique_ptr<Window> w) noexcept : m_win(std::move(w)) {}

 private:
  std::unique_ptr<Window> m_win;
};

class FrontendBuilder {
 public:
  using Self = FrontendBuilder;

  constexpr FrontendBuilder() noexcept = default;
  FrontendBuilder(const FrontendBuilder&) = delete;
  FrontendBuilder& operator=(const FrontendBuilder&) = delete;

  FrontendBuilder(FrontendBuilder&&) noexcept = default;
  FrontendBuilder& operator=(FrontendBuilder&&) noexcept = default;

  ~FrontendBuilder() = default;

  Self&& set_window(std::unique_ptr<Window> w) && {
    m_win = std::move(w);
    return std::move(*this);
  }
  Frontend commit() && { return Frontend(std::move(m_win)); }

 private:
  std::unique_ptr<Window> m_win{};
};

FrontendBuilder frontend(FrontendType type);
inline FrontendBuilder frontend(std::string_view nm) {
  return frontend(frontend_type(nm));
}

}  // namespace plai
