#pragma once

#include <algorithm>
#include <mutex>
#include <plai/flow/src.hpp>
#include <plai/logs/logs.hpp>
#include <plai/media/media.hpp>
#include <plai/store.hpp>

namespace plai::mods {

class MediaStore : public flow::Src<media::Media> {
 public:
    using Playlist = std::vector<std::string>;

    explicit MediaStore(Store& str) : m_str(&str) {}

    bool set_playlist(Playlist list) {
        flow::SrcSubscriber* sub{};
        {
            auto lk = std::lock_guard(m_mut);
            auto success = verify_playlist(list);
            if (!success) {
                PLAI_WARN("Playlist '{}' contains non-existent entries", list);
                return false;
            }
            m_playlist = std::move(list);
            m_iter = m_playlist.begin();
            sub = m_sub;
        }
        if (sub) sub->src_data_available();
        return true;
    }

    [[nodiscard]] std::span<const std::string> playlist() const noexcept {
        auto lk = std::lock_guard(m_mut);
        return m_playlist;
    }

    media::Media produce() override {
        auto lk = std::lock_guard(m_mut);
        auto iter = m_iter++;
        ++m_iter;
        if (m_iter == m_playlist.end()) m_iter = m_playlist.begin();
        return media::Image{m_str->read(*iter)};
    }

    bool data_available() override {
        auto lk = std::lock_guard(m_mut);
        return !m_playlist.empty();
    }

    void on_data_available(flow::SrcSubscriber* sub) override {
        auto lk = std::lock_guard(m_mut);
        m_sub = sub;
    }

 private:
    bool verify_playlist(std::span<const std::string> list) {
        auto existing = m_str->list();
        for (const auto& entry : list) {
            auto iter = std::find(existing.begin(), existing.end(), entry);
            if (iter == existing.end()) return false;
        }
        return true;
    }

    std::mutex m_mut{};
    Playlist m_playlist{};
    Playlist::const_iterator m_iter{m_playlist.begin()};
    Store* m_str{};
    flow::SrcSubscriber* m_sub{};
};
}  // namespace plai::mods
