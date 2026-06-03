#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <plai/store.hpp>
#include <plai/virtual.hpp>
#include <utility>

namespace plai::net {

enum class DeleteResult {
    Success,
    NotFound,
    Referenced,
};

struct MediaMeta {
    size_t size;
    crypto::Sha256 digest;
};

class ApiServer : public Virtual {
 public:
    virtual void run() = 0;
    virtual void stop() = 0;
};

class ApiV2 : public Virtual {
 public:
    // /_ping
    virtual void ping() {}

    virtual std::optional<MediaMeta> media_get(std::string_view key) = 0;

    enum class MediaPutStatus {
        Ok,
        Created,
    };

    /**
     * \brief Media upload
     * */
    virtual MediaPutStatus media_put(
        std::string_view key,
        std::function<std::optional<std::span<const uint8_t>>()> body) = 0;

    /**
     * \brief Delete a media
     *
     * \return True on success,
     * */
    virtual void media_delete(std::string_view key) = 0;

    /**
     * \brief List all get_medias
     *
     * \param type Whether to list images or videos or both (if std::nullopt).
     *
     * \return List of medias
     * */
    virtual std::vector<std::string> medias_get() = 0;

    /**
     * \brief Delete medias that are not referenced by any playlists
     * */
    virtual void medias_prune() = 0;

    struct PlaylistInfo {
        std::vector<std::string> medias;
        size_t window_size;
        size_t image_ms;
        bool active;
    };

    virtual std::optional<PlaylistInfo> playlist_info(std::string_view key) = 0;

    virtual void playlist_activate(std::string_view key, bool active) = 0;

    enum class Location {
        Front,
        Back,
        Next,
    };

    virtual void playlist_medias_append(
        std::string_view key, Location loc,
        std::span<const std::string> medias) = 0;

    virtual void playlist_medias_delete(
        std::string_view key, Location loc,
        std::span<const std::string> medias) = 0;

    virtual void playlist_medias_rotate(
        std::string_view key, Location loc,
        std::span<const std::string> medias) = 0;

    virtual void playlist_set_image_duration(std::string_view key,
                                             size_t image_ms) = 0;
};

class DefaultV2Api : public ApiV2 {
 public:
    std::optional<MediaMeta> media_get(std::string_view key) override;

    MediaPutStatus media_put(
        std::string_view key,
        std::function<std::optional<std::span<const uint8_t>>()> body) override;

    void media_delete(std::string_view key) override;

    std::vector<std::string> medias_get() override;

    void medias_prune() override;

    std::optional<PlaylistInfo> playlist_info(std::string_view key) override;

    void playlist_activate(std::string_view key, bool active) override;

    void playlist_medias_append(std::string_view key, Location loc,
                                std::span<const std::string> medias) override;

    void playlist_medias_delete(std::string_view key, Location loc,
                                std::span<const std::string> medias) override;

    void playlist_medias_rotate(std::string_view key, Location loc,
                                std::span<const std::string> medias) override;

    void playlist_set_image_duration(std::string_view key,
                                     size_t image_ms) override;

 private:
};

std::unique_ptr<ApiServer> launch_api(ApiV2* api, std::string_view bind);

}  // namespace plai::net
