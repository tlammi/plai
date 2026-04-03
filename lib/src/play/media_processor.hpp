#pragma once

#include <plai/frac.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/frame_converter.hpp>
#include <plai/media/hwaccel.hpp>
#include <plai/media/media.hpp>
#include <plai/ring_buffer.hpp>
#include <thread>

namespace plai::play {

/**
 * \brief Async media -> frame processor
 *
 * This processes medias to sequences of events.
 * */
class MediaProcessor {
    // Number of frames preprocessed
    static constexpr auto BUFFER_SIZE = 8;

 public:
    /**
     * \brief Input provider for the processor
     *
     * These methods are called from the processor's internal thread(s).
     * */
    class Input {
     public:
        /**
         * \brief Load next media
         *
         * This should block until media is available. Cancellation should be
         * indicated by throwing Cancelled.
         * */
        virtual media::Media next_media() = 0;

     protected:
        ~Input() = default;
    };
    /**
     * \brief Consumer for the processor
     *
     * These methods are called from the thread calling \a consume_next()
     * */
    class Output {
     public:
        /**
         * \brief Called to indicate start of a new media
         *
         * \param frm First frame of the new media
         * \param still True for images, false for videos
         * \param fps media FPS, can be ignored for images
         * */
        virtual void new_media(media::Frame frm, bool still, Frac<int> fps) = 0;

        /**
         * \brief Called for frames after the initial frame
         *
         * Naturally, this is only called for videos
         * */
        virtual void new_frame(media::Frame frm) = 0;

     protected:
        ~Output() = default;
    };

    struct Opts {
        media::HwAccel hwaccel{};
    };

    MediaProcessor(Input& input, Output& output, Opts opts = {.hwaccel = {}})
        : m_in(&input), m_out(&output), m_accel(opts.hwaccel) {}

    MediaProcessor(const MediaProcessor&) = delete;
    MediaProcessor& operator=(const MediaProcessor&) = delete;

    MediaProcessor(MediaProcessor&&) = delete;
    MediaProcessor& operator=(MediaProcessor&&) = delete;

    ~MediaProcessor() = default;

    /**
     * \brief Consume next event
     *
     * This loops back to Output instance passed to constructor
     *
     * \return True if there was something to do, false otherwise
     * */
    bool consume_next();

    void set_dims(Vec<int> dims) {
        auto lk = std::lock_guard(m_mut);
        m_dims = dims;
    }

 private:
    struct Meta {
        Frac<int> fps{};
        bool still{};
    };
    void work(std::stop_token st);

    std::mutex m_mut{};
    Input* m_in;
    Output* m_out;
    media::HwAccel m_accel;
    RingBuffer<media::Frame> m_buf{BUFFER_SIZE};
    RingBuffer<Meta> m_meta{BUFFER_SIZE};
    Vec<int> m_dims{};
    media::FrameConverter m_conv{};
    std::jthread m_worker{[&](std::stop_token tok) { work(tok); }};
};
}  // namespace plai::play
