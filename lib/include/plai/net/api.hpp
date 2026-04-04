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
    Failure,
};

struct MediaMeta {
    size_t size;
    crypto::Sha256 digest;
};

/**
 * \brief API implementation
 *
 * Bunch of callbacks called by the API server.
 *
 * See doc/rest.md for API specification
 * */
class ApiV1 : public Virtual {
 public:
    // /_ping
    virtual void ping() {}

    virtual MediaMeta media_get(std::string_view key) = 0;

    /**
     * \brief Media upload
     * */
    virtual void media_put(
        std::string_view key,
        std::function<std::optional<std::span<const uint8_t>>()> body) = 0;

    /**
     * \brief Delete a media
     *
     * \return
     *    Success - deleted
     *    Scheduled - marked for deletion and will be done later
     *    Failed - does not exist
     * */
    virtual DeleteResult media_delete(std::string_view key) = 0;

    /**
     * \brief List all get_medias
     *
     * \param type Whether to list images or videos or both (if std::nullopt).
     *
     * \return List of medias
     * */
    virtual std::vector<std::string> get_medias() = 0;

    /**
     * \brief Play medias
     *
     * \param medias List of medias to play
     * */
    // TODO: Should this have a return value to indicate success/failure?
    virtual void play(const std::vector<std::string>& medias, bool replay) = 0;
};

/**
 * \brief API implementing the media storing
 *
 * This uses the given Store object to handle storage of the medias. The play()
 * method is left for the user.
 * */
class DefaultApi : public ApiV1 {
 public:
    explicit DefaultApi(Store* store) : m_store(store) { assert(store); }

    MediaMeta media_get(std::string_view key) override;

    void media_put(
        std::string_view key,
        std::function<std::optional<std::span<const uint8_t>>()> body) override;

    DeleteResult media_delete(std::string_view key) override;

    std::vector<std::string> get_medias() override;

 private:
    Store* m_store;
};

class ApiServer : public Virtual {
 public:
    virtual void run() = 0;
    virtual void stop() = 0;
};

std::unique_ptr<ApiServer> launch_api(ApiV1* api, std::string_view bind);

}  // namespace plai::net
