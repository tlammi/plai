
#include <CLI/CLI.hpp>
#include <plai/store.hpp>
#include <print>

int main(int argc, char** argv) {
    std::string db{":memory:"};
    std::string key{};
    std::string value{};
    CLI::App app("store testing tool");
    argv = app.ensure_utf8(argv);
    app.add_option("-d,--db", db, "Database path")->default_str(db);
    app.require_subcommand(true);
    auto* ls = app.add_subcommand("ls", "List database entries");
    auto* inspect =
        app.add_subcommand("inspect", "Get metadata from a database entry");
    inspect->add_option("key", key, "Key for the entry to inspect");
    auto* store = app.add_subcommand("store", "Store data in the database");
    store->add_option("key", key, "Key for the entry");
    store->add_option("value", value, "Value to store");

    CLI11_PARSE(app, argc, argv);

    auto strg = plai::sqlite_store(db);
    if (store->count()) {
        strg->store(key,
                    std::span(reinterpret_cast<const uint8_t*>(value.data()),
                              value.size()));
    } else if (ls->count()) {
        auto res = strg->list();
        for (const auto& k : res) { std::println("{}", k); }
    }
    return EXIT_SUCCESS;
}
