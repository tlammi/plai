#include <plai/media/exceptions.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace plai::media {
namespace {
std::string av_error_string(int code) {
    std::string buf = std::string(AV_ERROR_MAX_STRING_SIZE, '\0');
    av_make_error_string(buf.data(), buf.size(), code);
    return buf;
}
}  // namespace

AVException::AVException(int ec) noexcept
    : std::runtime_error(av_error_string(ec)) {}

}  // namespace plai::media
