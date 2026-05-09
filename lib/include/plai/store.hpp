#pragma once

#include <memory>
#include <optional>
#include <plai/c_str.hpp>
#include <plai/crypto.hpp>
#include <plai/virtual.hpp>
#include <utility>
#include <vector>

namespace plai {

struct BlobMeta {
    size_t bytes;
    crypto::Sha256 sha256;
    bool locked;
    bool marked_for_deletion;
};

class Store : public Virtual {
 public:
    /**
     * \brief List keys in the storage
     * */
    virtual std::vector<std::string> list() = 0;

    /**
     * \brief Add a blob to the store
     *
     *
     * \param key Key for the data
     * \param blob Data to store
     * */
    virtual void store(CStr key, std::span<const uint8_t> blob) = 0;

    /**
     * \brief Get metadata about a blob
     *
     * \param key Name of the data
     *
     * \return BlobMeta on success, std::nullopt if the data could not be found
     * */
    virtual std::optional<BlobMeta> inspect(CStr key) = 0;

    /**
     * \brief Lock blobs so they cannot be deleted
     *
     * Locked blobs are preserved until they are unlocked. The operation is
     * atomic: on success all the blobs are locked and on failure none are.
     *
     * \param keys Keys of the blobs to lock
     *
     * \return true if lock was successfull, false on failure
     * */
    virtual bool lock(std::span<CStr> keys) = 0;

    /**
     * \brief Unlock blobs
     *
     * Unlock blobs. Blobs that are not blocked are ignored.
     * All blobs scheduled for deletion and unblocked here are deleted.
     *
     * \param keys Keys of the blobs to unlock
     * */
    virtual void unlock(std::span<CStr> keys) = 0;

    /**
     * \brief Read a blob
     * */
    virtual std::vector<uint8_t> read(CStr key) = 0;

    /**
     * \brief Remove a blob
     *
     * This either deletes a blob immediately or marks it for deletion later in
     * case the blob is locked. Such blobs are deleted once the lock is
     * released.
     *
     * \param key Key of the blob to delete
     * */
    virtual void remove(CStr key) = 0;
};

class Transaction;

class Transactional : public Virtual {
    friend Transaction;

 public:
    virtual Transaction begin_transaction() = 0;

 private:
    virtual void commit_transaction() noexcept = 0;
    virtual void cancel_transaction() noexcept = 0;
};

class Transaction {
 public:
    constexpr Transaction() noexcept = default;
    constexpr explicit Transaction(Transactional& parent) noexcept
        : m_parent(&parent) {}

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    constexpr Transaction(Transaction&& other) noexcept
        : m_parent(std::exchange(other.m_parent, nullptr)) {}

    constexpr Transaction& operator=(Transaction&& other) noexcept {
        std::destroy_at(this);
        std::construct_at(this, std::move(other));
        return *this;
    }

    constexpr ~Transaction() noexcept {
        auto* parent = std::exchange(m_parent, nullptr);
        if (parent) parent->cancel_transaction();
    }

    void commit() noexcept {
        auto* parent = std::exchange(m_parent, nullptr);
        if (parent) parent->commit_transaction();
    }

 private:
    Transactional* m_parent{};
};

struct MediaMeta {
    size_t size;
    crypto::Sha256 digest;
};

enum class MediaDelStatus {
    Success,     // Ok
    NotFound,    // No such media
    Referenced,  // Used by a playlist
};

struct PlaylistSpec {
    std::vector<std::string> items{};
    bool active{};
};

class Store2 : public Virtual {
 public:
    virtual Transaction begin_transaction() = 0;

    virtual std::vector<std::string> medias() = 0;

    virtual void media_set(CStr key, std::span<const std::byte> data) = 0;
    virtual MediaDelStatus media_delete(CStr key) = 0;
    virtual std::optional<MediaMeta> media_meta(CStr key);
    virtual std::optional<std::vector<std::byte>> media_get(CStr key) = 0;
};

std::unique_ptr<Store> sqlite_store(CStr path);

}  // namespace plai
