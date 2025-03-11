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

    void put_media(MediaType type, std::string_view key,
                   std::function<std::optional<std::span<const uint8_t>>()>
                       body) override {
        while (true) {
            auto r = body();
            if (!r) {
                std::println("media done");
                return;
            }
            std::println("media chunk: {}", r->size());
        }
    }

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
}
