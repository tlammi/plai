#pragma once

#include <plai/media/forward.hpp>
#include <plai/vec.hpp>

namespace plai::media {
class Frame {
 public:
    Frame();
    constexpr explicit Frame(AVFrame* raw) noexcept : m_raw(raw) {}

    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;

    Frame(Frame&& other) noexcept;
    Frame& operator=(Frame&& other) noexcept;

    ~Frame();

    [[nodiscard]] AVFrame* raw() noexcept { return m_raw; }
    [[nodiscard]] const AVFrame* raw() const noexcept { return m_raw; }

    /**
     * \brief False for empty frames
     * */
    explicit operator bool() const noexcept;

    int width() const noexcept;
    int height() const noexcept;
    Vec<int> dims() const noexcept { return {width(), height()}; }

    // Implementation detail. Don't touch.
    constexpr void is_dynamic(bool v) noexcept { m_is_dyn = v; }

    constexpr bool is_dynamic() const noexcept { return m_is_dyn; }

 private:
    AVFrame* m_raw;
    // Hack to get libswscale working.
    //
    // It does some special memory allocation which means we need to do some
    // special memory deallocation.
    bool m_is_dyn{false};
};
}  // namespace plai::media
