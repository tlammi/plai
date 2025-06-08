#pragma once

#include <plai/flow/sink.hpp>
#include <plai/flow/src.hpp>
#include <plai/frac.hpp>
#include <plai/media/frame.hpp>
#include <plai/media/media.hpp>
#include <plai/ring_buffer.hpp>

namespace plai::mods {

struct DecodingMeta {
    Frac<int> fps{};
    constexpr bool still() const noexcept { return fps.is_nan(); }
};

using Decoded = std::variant<DecodingMeta, media::Frame>;

/**
 * \brief Module for converting media blobs to streams of frames
 *
 * Stream for each meta starts with an instance of DeocingMeta followed by one
 * or more frames.
 * */
class Decoder : public flow::Sink<media::Media>, public flow::Src<Decoded> {};

std::unique_ptr<Decoder> make_decoder(sched::Executor exec);

}  // namespace plai::mods
