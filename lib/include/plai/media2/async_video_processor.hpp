#pragma once

#include <exec/task.hpp>
#include <plai/ex/yield.hpp>
#include <plai/media/decoder.hpp>
#include <plai/media/demux.hpp>

namespace plai::media2 {

class ProcessingCtx {
 public:
    virtual ~ProcessingCtx() = 0;

    virtual Vec<int> dims() = 0;
    virtual media::HwAccel hwaccel() = 0;
    virtual std::span<const std::byte> data() = 0;

    virtual std::pair<size_t, media::StreamView> select_stream(
        media::Demux& demux) = 0;

    virtual void meta(bool is_still, Frac<int> fps) = 0;
    virtual void frame(media::Frame frm) = 0;
};

exec::task<void> process_video(ProcessingCtx* ctx);

}  // namespace plai::media2
