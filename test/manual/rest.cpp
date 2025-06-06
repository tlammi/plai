#include <plai/exceptions.hpp>
#include <plai/net/api.hpp>
#include <plai/store.hpp>
#include <plai/thirdparty/magic_enum.hpp>
#include <plai/util/str.hpp>
#include <print>

using plai::Store;
using plai::net::DeleteResult;
using plai::net::MediaListEntry;
using plai::net::MediaMeta;
using plai::net::MediaType;

class Api final : public plai::net::ApiV1 {
 public:
    Api(Store* store) : m_store(store) {}

    void ping() override {}

    MediaMeta get_media(MediaType type, std::string_view key) override {
        auto s =
            std::format("{}/{}", plai::net::serialize_media_type(type), key);
        auto res = m_store->inspect(s);
        return {
            .size = res->bytes,
            .digest = res->sha256,
        };
    }

    void put_media(MediaType type, std::string_view key,
                   std::function<std::optional<std::span<const uint8_t>>()>
                       body) override {
        std::vector<uint8_t> buf{};
        while (true) {
            auto r = body();
            if (!r) break;
            buf.insert(buf.end(), r->begin(), r->end());
        }
        std::println(stderr, "received total {} bytes", buf.size());
        auto s =
            std::format("{}/{}", plai::net::serialize_media_type(type), key);
        std::println(stderr, "storing as {}", s);
        m_store->store(s, buf);
    }

    DeleteResult delete_media(MediaType type, std::string_view key) override {
        auto s =
            std::format("{}/{}", plai::net::serialize_media_type(type), key);
        std::println(stderr, "deleting {}", s);
        // TODO: cannot get info whether was deleted. Do something
        m_store->remove(s);
        return DeleteResult::Success;
    }

    std::vector<MediaListEntry> get_medias(
        std::optional<MediaType> type) override {
        auto entries = m_store->list();
        std::vector<MediaListEntry> out{};
        out.reserve(entries.size());
        for (const auto& e : entries) {
            auto [type_name, key] = plai::split_left(e, "/");
            auto type_v = plai::net::parse_media_type(type_name);
            if (!type_v)
                throw std::runtime_error(
                    "Corrupted database: Invalid key prefix");
            out.emplace_back(*type_v, std::string(key));
        }
        return out;
    }

    void play(const std::vector<MediaListEntry>& medias, bool replay) override {
        (void)medias;
        (void)replay;
        std::println(stderr, "play called");
    }

 private:
    Store* m_store{};
};

int main(int argc, char** argv) {
    if (argc != 2) { throw plai::ValueError("usage: rest [socket]"); }
    auto store = plai::sqlite_store(":memory:");
    auto api = Api(store.get());
    auto srv = plai::net::launch_api(&api, argv[1]);
    srv->run();
}
