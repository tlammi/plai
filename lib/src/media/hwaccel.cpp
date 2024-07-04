#include <plai/media/hwaccel.hpp>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace plai::media {

const char* HwAccelMeta::name() const noexcept {
  return av_hwdevice_get_type_name(AVHWDeviceType(m_type));
}

auto HwAccelRange::Iter::operator++() noexcept -> Iter& {
  auto tp = av_hwdevice_iterate_types(AVHWDeviceType(m_meta.type()));
  m_meta = HwAccelMeta(tp);
  return *this;
}

auto HwAccelRange::begin() const noexcept -> Iter { return Iter{m_first}; }
auto HwAccelRange::end() const noexcept -> Iter { return Iter{}; }

HwAccelRange supported_hardware_accelerators() noexcept {
  auto tp = AV_HWDEVICE_TYPE_NONE;
  static_assert(AV_HWDEVICE_TYPE_NONE == 0);
  tp = av_hwdevice_iterate_types(tp);
  return HwAccelRange(tp);
}

}  // namespace plai::media
