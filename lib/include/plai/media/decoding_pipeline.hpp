#pragma once

#include <deque>
#include <filesystem>
#include <plai/media/decoding_stream.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/persist_buffer.hpp>
#include <plai/queue.hpp>
#include <thread>
#include <vector>

namespace plai::media {

namespace stdfs = std::filesystem;

class DecodingPipeline {
 public:
    using Media = std::variant<std::vector<uint8_t>, stdfs::path>;
    static constexpr size_t BUFFER_SIZE = 1024;
    DecodingPipeline() = default;

    DecodingPipeline(const DecodingPipeline&) = delete;
    DecodingPipeline& operator=(const DecodingPipeline&) = delete;

    DecodingPipeline(DecodingPipeline&&) = delete;
    DecodingPipeline& operator=(DecodingPipeline&&) = delete;

    ~DecodingPipeline();

    DecodingStream frame_stream();

    void decode(std::vector<uint8_t> data);
    void decode(stdfs::path path);

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

 private:
    void work(std::stop_token tok);

    PersistBuffer<Frame> m_buf{BUFFER_SIZE};
    std::mutex m_mut{};
    std::condition_variable m_cv{};
    std::deque<Media> m_medias{};
    Queue<Frac<int>> m_framerates{};
    Vec<int> m_dims{};
    FrameConverter m_conv{};
    std::jthread m_worker{[&](std::stop_token tok) { work(tok); }};
};
}  // namespace plai::media
