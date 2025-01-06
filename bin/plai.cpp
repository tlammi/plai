#include <plai/store.hpp>
#include <print>

#include "cli.hpp"

namespace plaibin {

int do_main(int argc, char** argv) {
    try {
        auto args = parse_cli(argc, argv);
        auto store = plai::sqlite_store(args.db);
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

