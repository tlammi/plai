#include <sqlite3.h>

#include <cassert>
#include <plai/crypto.hpp>
#include <plai/store.hpp>

#include "sqlite.hpp"

namespace plai {
namespace {
constexpr CStr create_tbl_stmt =
    "CREATE TABLE IF NOT EXISTS plai (name STRING UNIQUE, sha256 BLOB, bytes "
    "INTEGER, data "
    "BLOB);";
constexpr CStr list_stmt = "SELECT (name) from plai;";
constexpr CStr store_stmt = "INSERT INTO plai VALUES (?,?,?,?);";
constexpr CStr inspect_stmt =
    "SELECT (sha256, bytes) FROM plai WHERE (name=?);";
constexpr CStr read_stmt = "SELECT (data) FROM plai WHERE (name=?);";
constexpr CStr rm_stmt = "DELETE FROM plai WHERE (name=?);";

}  // namespace

class SqliteStore final : public Store {
 public:
    explicit SqliteStore(CStr path) : m_conn(sqlite::connect(path)) {
        auto stmt = sqlite::statement(m_conn.get(), create_tbl_stmt);
        while (true) {
            int res = sqlite3_step(stmt.get());
            if (res == SQLITE_DONE) break;
            if (res == SQLITE_BUSY) continue;
            throw sqlite::SqliteException(m_conn.get());
        }
    }

    std::vector<std::string> list() final {
        auto stmt = sqlite::statement(m_conn.get(), list_stmt);
        std::vector<std::string> out{};
        while (true) {
            auto res = sqlite::step_one(m_conn, stmt);
            if (res == SQLITE_DONE) break;
            if (res == SQLITE_BUSY) continue;
            if (res == SQLITE_ROW) {
                out.emplace_back(sqlite::unbind<std::string>(m_conn, stmt, 0));
                continue;
            }
            throw ValueError("failed listing entries");
        }
        return out;
    }

    void store(CStr key, std::span<const uint8_t> blob) final {
        auto stmt = sqlite::statement(m_conn.get(), store_stmt);
        auto digest = crypto::sha256(blob);
        sqlite::bind_all(m_conn, stmt, key, digest, blob.size(), blob);
        sqlite::step_all(m_conn, stmt);
    }

    std::optional<BlobMeta> inspect(CStr key) final {
        auto stmt = sqlite::statement(m_conn.get(), inspect_stmt);
        sqlite::bind_all(m_conn, stmt, key);
        int res = SQLITE_BUSY;
        while (res == SQLITE_BUSY) { res = sqlite::step_one(m_conn, stmt); }
        if (res == SQLITE_DONE) return std::nullopt;
        assert(res == SQLITE_ROW);
        auto values =
            sqlite::unbind_all<std::vector<uint8_t>, int64_t>(m_conn, stmt);
        sqlite::step_all(m_conn, stmt);
        return BlobMeta{
            .bytes = static_cast<size_t>(std::move(std::get<1>(values))),
            .sha256 = std::move(std::get<0>(values)),
        };
    }

    bool lock(std::span<CStr> keys) final {}

    void unlock(std::span<CStr> keys) final {}

    std::vector<uint8_t> read(CStr key) final {
        auto stmt = sqlite::statement(m_conn.get(), read_stmt);
        sqlite::bind_all(m_conn, stmt, key);
        int res = SQLITE_BUSY;
        while (res == SQLITE_BUSY) { res = sqlite::step_one(m_conn, stmt); }
        if (res != SQLITE_ROW)
            throw ValueError(std::format("no data in storage matching key '{}'",
                                         key.view()));
        auto data = sqlite::unbind_all<std::vector<uint8_t>>(m_conn, stmt);
        sqlite::step_all(m_conn, stmt);
        return std::get<0>(std::move(data));
    }

    void remove(CStr key) final {
        auto stmt = sqlite::statement(m_conn.get(), rm_stmt);
        sqlite::bind_all(m_conn, stmt, key);
        sqlite::step_all(m_conn, stmt);
    }

 private:
    sqlite::Connection m_conn;
};
std::unique_ptr<Store> sqlite_store(CStr path) {
    return std::make_unique<SqliteStore>(path);
}
}  // namespace plai
