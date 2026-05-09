#include <map>
#include <plai/ctx/media_store.hpp>

namespace plai::ctx {

class SimpleMediaStore final : public MediaStore {
 public:
    void set(CStr nm, std::span<const std::byte> data) override {
        m_map[std::string(nm)] =
            media::Media{std::vector(data.begin(), data.end())};
    }

    bool erase(CStr nm) override {
        auto it = m_map.find(nm);
        if (it == m_map.end()) return false;
        m_map.erase(it);
        return true;
    }

    bool contains(CStr nm) override { return m_map.contains(nm); }

    media::Media get(CStr nm) override {
        auto it = m_map.find(nm);
        if (it == m_map.end()) return media::Media{};
        return it->second;
    }

 private:
    std::map<std::string, media::Media, std::less<>> m_map{};
};

std::unique_ptr<MediaStore> simple_media_store() {
    return std::make_unique<SimpleMediaStore>();
}
}  // namespace plai::ctx
