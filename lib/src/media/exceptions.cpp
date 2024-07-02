#include <plai/media/exceptions.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace plai::media {

AVException::AVException(int ec) noexcept
    : std::runtime_error(av_err2str(ec)) {}

}  // namespace plai::media
