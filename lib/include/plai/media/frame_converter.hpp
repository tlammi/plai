#pragma once

#include <plai/media/forward.hpp>
#include <plai/media/frame.hpp>
#include <plai/vec.hpp>

namespace plai::media {

class FrameConverter {
 public:
    FrameConverter() noexcept = default;

    FrameConverter(const FrameConverter&) = delete;
    FrameConverter& operator=(const FrameConverter&) = delete;

    FrameConverter(FrameConverter&& other) noexcept
        : m_ctx(std::exchange(other.m_ctx, nullptr)) {}
    FrameConverter& operator=(FrameConverter&& other) noexcept {
        auto tmp = FrameConverter(std::move(other));
        std::swap(m_ctx, tmp.m_ctx);
        return *this;
    }

    ~FrameConverter();

    /**
     * Convert a frame
     *
     * \param dst_dims Destination frame dimensions
     * \param src Frame to convert
     * \param dst Optional frame to overwrite possibly avoiding
     * allocation
     * \return Converted frame
     * */
    Frame operator()(Vec<int> dst_dims, const Frame& src, Frame dst = {});

 private:
    SwsContext* m_ctx{};
};

}  // namespace plai::media
