#pragma once

#include <deque>
#include <filesystem>
#include <plai/media/decoding_stream.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/media/hwaccel.hpp>
#include <plai/media/media.hpp>
#include <plai/persist_buffer.hpp>
#include <plai/queue.hpp>
#include <thread>
#include <vector>

namespace plai::media {

namespace stdfs = std::filesystem;

class DecodingPipeline {
 public:
    using Media = plai::media::Media;

    static constexpr size_t BUFFER_SIZE = 8;
    explicit DecodingPipeline(HwAccel accel = {}) : m_accel(std::move(accel)) {}

    DecodingPipeline(const DecodingPipeline&) = delete;
    DecodingPipeline& operator=(const DecodingPipeline&) = delete;

    DecodingPipeline(DecodingPipeline&&) = delete;
    DecodingPipeline& operator=(DecodingPipeline&&) = delete;

    ~DecodingPipeline();

    DecodingStream frame_stream();

    void decode(std::vector<uint8_t> data);
    void decode(stdfs::path path);

    /**
     * \brief Clear the enqueued medias
     *
     * This is typically used in conjunction with playlist change to make the
     * switch faster.
     * */
    void clear_media_queue();

    /**
     * \brief Set target dimensions
     *
     * Set target frame dimensions. This can be used for lowering
     * the frame resolution so less work needs to be done. Set to {0, 0}
     * to disable the optimization
     * */
    void set_dims(Vec<int> dims) noexcept {
        std::unique_lock lk{m_mut};
        m_dims = dims;
    }

    /**
     * \brief Number of medias waiting for decoding
     * */
    size_t queued_medias() const noexcept;

 private:
    struct Meta {
        Frac<int> fps;
        bool still{};
    };

    void work(std::stop_token tok);

    mutable std::mutex m_mut{};
    HwAccel m_accel{};
    RingBuffer<Frame> m_buf{BUFFER_SIZE};
    std::condition_variable m_cv{};
    std::deque<std::vector<uint8_t>> m_medias{};
    Queue<Meta> m_metas{};
    Vec<int> m_dims{};
    FrameConverter m_conv{};
    std::jthread m_worker{[&](std::stop_token tok) { work(tok); }};
};
}  // namespace plai::media
