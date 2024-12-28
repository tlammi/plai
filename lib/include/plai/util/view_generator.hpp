#pragma once

#include <functional>
#include <optional>
#include <ranges>
#include <type_traits>

namespace plai {
namespace detail {

template <class T>
struct is_optional : std::false_type {};

template <class T>
struct is_optional<std::optional<T>> : std::true_type {};

template <class T>
constexpr bool is_optional_v = is_optional<T>::value;

}  // namespace detail

template <class Func>
class ViewGenerator : public std::ranges::view_interface<ViewGenerator<Func>> {
 public:
    class Iterator {
        using Value = std::invoke_result_t<Func>;

     public:
        using value_type = typename Value::value_type;
        using difference_type = std::ptrdiff_t;
        constexpr Iterator() = default;

        constexpr explicit Iterator(Func* f) noexcept
            : m_f(f), m_val((*m_f)()) {}

        const auto& operator*() const { return *m_val; }
        auto& operator++() {
            m_val = (*m_f)();
            return *this;
        }
        void operator++(int) { ++m_val; }

        constexpr bool operator==(
            std::default_sentinel_t /*unused*/) const noexcept {
            return !m_val;
        }

     private:
        Func* m_f{};
        Value m_val{};
        static_assert(
            detail::is_optional_v<Value>,
            "Callable needs to have a prototype () -> std::optional<T>");
    };

    constexpr explicit ViewGenerator(Func f) : m_f(f) {}

    auto begin() { return Iterator(&m_f); }
    auto end() { return std::default_sentinel; }

 private:
    Func m_f;
};

template <class F>
auto view_generator(F&& f) {
    return ViewGenerator(std::forward<F>(f));
}

}  // namespace plai
