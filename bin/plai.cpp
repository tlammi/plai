#include <plai.hpp>
#include <print>

#include "cli.hpp"

namespace plaibin {

class ApiImpl : public plai::net::DefaultApi {
    using Parent = plai::net::DefaultApi;

 public:
    explicit ApiImpl(plai::Store* store) : Parent(store) {}

    void play(const std::vector<plai::net::MediaListEntry>& medias,
              bool replay) override {
        std::println(stderr, "'/play' not implemented");
    }
};

int run(const Cli& args) {
    auto store = plai::sqlite_store(args.db);
    auto api = ApiImpl(store.get());
    auto srv = plai::net::launch_api(&api, args.socket);
    srv->run();
    return 0;
}

int do_main(int argc, char** argv) {
    try {
        return run(parse_cli(argc, argv));
    } catch (const Exit& e) {
        return e.code();
    } catch (const std::exception& e) {
        std::println(stderr, "{}", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace plaibin
int main(int argc, char** argv) { ::plaibin::do_main(argc, argv); }
