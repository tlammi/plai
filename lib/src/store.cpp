#include <sqlite3.h>

#include <cassert>
#include <format>
#include <plai/crypto.hpp>
#include <plai/store.hpp>

#include "sqlite.hpp"

namespace plai {
namespace {
constexpr CStr create_tbl_stmt =
    "CREATE TABLE IF NOT EXISTS plai (name STRING PRIMARY KEY, sha256 BLOB, "
    "bytes INTEGER, locked BOOLEAN, marked_for_deletion BOOLEAN, data BLOB);";
constexpr CStr list_stmt = "SELECT (name) from plai;";
constexpr CStr store_stmt = "INSERT OR REPLACE INTO plai VALUES (?,?,?,0,0,?);";
constexpr CStr inspect_stmt =
    "SELECT sha256, bytes, locked, marked_for_deletion FROM plai WHERE "
    "(name=?);";
constexpr CStr read_stmt = "SELECT (data) FROM plai WHERE (name=?);";
constexpr CStr mark_for_deletion_stmt =
    "UPDATE plai SET marked_for_deletion=? WHERE name=?;";
constexpr CStr prune_marked_stmt =
    "DELETE FROM plai WHERE (marked_for_deletion=1 AND locked=0);";

std::string lock_stmt(std::span<CStr> keys, bool lock) {
    assert(!keys.empty());
    std::string stmt =
        std::format("UPDATE plai SET locked={} WHERE name=\"{}\"", lock ? 1 : 0,
                    keys.front().view());
    keys = keys.subspan(1);
    while (!keys.empty()) {
        stmt += std::format(" OR name=\"{}\"", keys.front().view());
        keys = keys.subspan(1);
    }
    stmt += ";";
    return stmt;
}

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
            sqlite::unbind_all<crypto::Sha256, int64_t, int64_t, int64_t>(
                m_conn, stmt);
        sqlite::step_all(m_conn, stmt);
        return BlobMeta{
            .bytes = static_cast<size_t>(std::move(std::get<1>(values))),
            .sha256 = std::move(std::get<0>(values)),
            .locked = static_cast<bool>(std::get<2>(values)),
            .marked_for_deletion = static_cast<bool>(std::get<3>(values)),
        };
    }

    bool lock(std::span<CStr> keys) final {
        auto stmt = sqlite::statement(m_conn.get(), lock_stmt(keys, true));
        sqlite::step_all(m_conn, stmt);
        auto ls = list();
        for (const auto& k : keys) {
            auto iter = std::find(ls.begin(), ls.end(), k);
            if (iter == ls.end()) {
                unlock(keys);
                return false;
            }
        }
        return true;
    }

    void unlock(std::span<CStr> keys) final {
        auto stmt = sqlite::statement(m_conn.get(), lock_stmt(keys, false));
        sqlite::step_all(m_conn, stmt);
        stmt = sqlite::statement(m_conn.get(), prune_marked_stmt);
        sqlite::step_all(m_conn, stmt);
    }

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
        auto stmt = sqlite::statement(m_conn.get(), mark_for_deletion_stmt);
        sqlite::bind_all(m_conn, stmt, 1, key);
        sqlite::step_all(m_conn, stmt);
        stmt = sqlite::statement(m_conn.get(), prune_marked_stmt);
        sqlite::step_all(m_conn, stmt);
    }

 private:
    sqlite::Connection m_conn;
};
std::unique_ptr<Store> sqlite_store(CStr path) {
    return std::make_unique<SqliteStore>(path);
}
}  // namespace plai
