#pragma once

#include <sqlite3.h>

#include <memory>
#include <plai/c_str.hpp>
#include <plai/crypto.hpp>
#include <plai/exceptions.hpp>
#include <ranges>
#include <vector>

namespace plai::sqlite {

class SqliteException : public Exception {
 public:
    SqliteException(sqlite3* handle) : Exception(sqlite3_errmsg(handle)) {}
    SqliteException(CStr str) : Exception(str) {}
};

void check_error(int code, sqlite3* handle) {
    if (code != SQLITE_OK) throw SqliteException(handle);
}

using Connection = std::unique_ptr<sqlite3, int (*)(sqlite3*)>;
inline Connection connect(CStr path) {
    sqlite3* handle{};
    auto res = sqlite3_open_v2(
        path, &handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (!handle) throw std::bad_alloc();
    auto conn = Connection(handle, &sqlite3_close);
    check_error(res, handle);
    return conn;
}

using Statement = std::unique_ptr<sqlite3_stmt, int (*)(sqlite3_stmt*)>;
inline Statement statement(sqlite3* db, CStr stmt) {
    sqlite3_stmt* s{};
    int res = sqlite3_prepare_v2(db, stmt, -1, &s, nullptr);
    check_error(res, db);
    return {s, &sqlite3_finalize};
}

inline void bind(Connection& conn, Statement& stmt, int idx,
                 std::span<const uint8_t> blob) {
    int res = sqlite3_bind_blob64(stmt.get(), idx, blob.data(),
                                  static_cast<int>(blob.size()), SQLITE_STATIC);
    check_error(res, conn.get());
}

inline void bind(Connection& conn, Statement& stmt, int idx, CStr str) {
    int res = sqlite3_bind_text(stmt.get(), idx, str,
                                static_cast<int>(str.size()), SQLITE_STATIC);
    check_error(res, conn.get());
}

inline void bind(Connection& conn, Statement& stmt, int idx, int64_t val) {
    int res = sqlite3_bind_int64(stmt.get(), idx, val);
    check_error(res, conn.get());
}

namespace detail {

template <int Idx, class T, class... Ts>
void bind_all_impl(Connection& conn, Statement& stmt, T&& first, Ts&&... rest) {
    bind(conn, stmt, Idx, std::forward<T>(first));
    if constexpr (sizeof...(Ts))
        bind_all_impl<Idx + 1, Ts...>(conn, stmt, std::forward<Ts>(rest)...);
}
}  // namespace detail

template <class... Ts>
void bind_all(Connection& conn, Statement& stmt, Ts&&... ts) {
    detail::bind_all_impl<1, Ts...>(conn, stmt, std::forward<Ts>(ts)...);
}

template <class T>
T unbind(Connection& conn, Statement& stmt, int idx);

template <>
std::vector<uint8_t> unbind<std::vector<uint8_t>>(Connection& conn,
                                                  Statement& stmt, int idx) {
    const void* ptr = sqlite3_column_blob(stmt.get(), idx);
    const auto bytes = sqlite3_column_bytes(stmt.get(), idx);
    auto span =
        std::span<const uint8_t>(static_cast<const uint8_t*>(ptr), bytes);
    return {span.begin(), span.end()};
}

template <>
crypto::Sha256 unbind<crypto::Sha256>(Connection& conn, Statement& stmt,
                                      int idx) {
    const void* ptr = sqlite3_column_blob(stmt.get(), idx);
    const auto bytes = sqlite3_column_bytes(stmt.get(), idx);
    static constexpr size_t expected_size = crypto::Sha256().size();

    if (bytes != expected_size) {
        SqliteException("invalid SHA256 checksum stored");
    }
    auto span =
        std::span<const uint8_t>(static_cast<const uint8_t*>(ptr), bytes);
    crypto::Sha256 out{};
    std::copy(span.begin(), span.end(), out.begin());
    return out;
}

template <>
std::string unbind<std::string>(Connection& conn, Statement& stmt, int idx) {
    const uint8_t* str = sqlite3_column_text(stmt.get(), idx);
    const auto bytes = sqlite3_column_bytes(stmt.get(), idx);
    // NOLINTNEXTLINE
    return std::string(reinterpret_cast<const char*>(str), bytes);
}

template <>
int64_t unbind<int64_t>(Connection& conn, Statement& stmt, int idx) {
    return sqlite3_column_int64(stmt.get(), idx);
}

namespace detail {

template <class T>
auto unbind_and_inc(Connection& conn, Statement& stmt, int* idx) {
    return unbind<T>(conn, stmt, (*idx)++);
}
}  // namespace detail

template <class... Ts>
std::tuple<Ts...> unbind_all(Connection& conn, Statement& stmt) {
    int idx = 0;
    return {detail::unbind_and_inc<Ts>(conn, stmt, &idx)...};
}

inline auto step_one(Connection& conn, Statement& stmt) {
    auto res = sqlite3_step(stmt.get());
    switch (res) {
        case SQLITE_DONE:
        case SQLITE_BUSY:
        case SQLITE_ROW: return res;
        default: throw SqliteException(conn.get());
    }
}

inline void step_all(Connection& conn, Statement& stmt) {
    while (true) {
        auto res = step_one(conn, stmt);
        if (res == SQLITE_DONE) return;
    }
}

}  // namespace plai::sqlite
