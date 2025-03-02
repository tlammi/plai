#include <plai/exceptions.hpp>
#include <plai/net/api.hpp>
#include <print>

using plai::net::DeleteResult;
using plai::net::MediaListEntry;
using plai::net::MediaMeta;
using plai::net::MediaType;

class Api final : public plai::net::ApiV1 {
 public:
    void ping() override {}
    MediaMeta get_media(MediaType type, std::string_view key) override {}

    void put_media(
        MediaType type, std::string_view key,
        std::function<void(std::span<const uint8_t>)> body) override {}

    DeleteResult delete_media(MediaType type, std::string_view key) override {}

    std::vector<MediaListEntry> get_medias(
        std::optional<MediaType> type) override {}

    void play(const std::vector<MediaListEntry>& medias, bool replay) override {
    }

 private:
};

int main(int argc, char** argv) {
    if (argc != 2) { throw plai::ValueError("usage: rest [socket]"); }
    auto api = Api();
    auto srv = plai::net::launch_api(&api, argv[1]);
    srv->run();
#if 0
    if (argc != 2) { throw plai::ValueError("usage: rest [socket]"); }
    auto builder = plai::net::http::server();
    builder.prefix("/plai/v1");
    std::map<std::string, std::string> dict{};
    auto srv =
        builder
            .service("/store/{key}", plai::net::http::METHOD_POST,
                     [&](const Request& req) -> Response {
                         std::println("data: {}", req.body);
                         dict[std::string(req.target.at("key"))] =
                             std::string(req.body);
                         return {};
                     })
            .service("/read/{key}", plai::net::http::METHOD_GET,
                     [&](const Request& req) -> Response {
                         return {dict[std::string(req.target.at("key"))]};
                     })
            .bind(argv[1])
            .commit();
    srv.run();
#endif
}
