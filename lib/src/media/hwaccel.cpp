#include <plai/media/hwaccel.hpp>
#include <utility>

#include "av_check.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace plai::media {
namespace {
AVBufferRef* alloc_hwdevice_ctx(AVHWDeviceType type) {
    AVBufferRef* ctx{};
    AV_CHECK(av_hwdevice_ctx_create(&ctx, type, nullptr, nullptr, 0));
    return ctx;
}
}  // namespace

const char* HwAccelMeta::name() const noexcept {
    return av_hwdevice_get_type_name(AVHWDeviceType(m_type));
}

HwAccel::HwAccel(hwaccel_t type)
    : m_ref(alloc_hwdevice_ctx(static_cast<AVHWDeviceType>(type))) {}

HwAccel::HwAccel(const HwAccel& other)
    : m_ref(other.m_ref ? av_buffer_ref(other.m_ref) : nullptr) {}

HwAccel& HwAccel::operator=(const HwAccel& other) {
    auto tmp = std::move(*this);
    m_ref = other.m_ref ? av_buffer_ref(other.m_ref) : nullptr;
    return *this;
}

HwAccel::HwAccel(HwAccel&& other) noexcept
    : m_ref(std::exchange(other.m_ref, nullptr)) {}

HwAccel& HwAccel::operator=(HwAccel&& other) noexcept {
    auto tmp = std::move(other);
    std::swap(m_ref, tmp.m_ref);
    return *this;
}

HwAccel::~HwAccel() {
    if (m_ref) av_buffer_unref(&m_ref);
}

hwaccel_t HwAccel::type() const noexcept {
    if (!m_ref) return AV_HWDEVICE_TYPE_NONE;
    // NOLINTNEXTLINE
    return reinterpret_cast<const AVHWDeviceContext*>(m_ref)->type;
}

HwAccel::operator bool() const noexcept {
    return static_cast<AVHWDeviceType>(type()) != AV_HWDEVICE_TYPE_NONE;
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
