#pragma once

#include <memory>
#include <plai/c_str.hpp>
#include <plai/virtual.hpp>
#include <vector>

namespace plai {

struct BlobMeta {
    size_t bytes;
    std::vector<uint8_t> sha256;
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

std::unique_ptr<Store> sqlite_store(CStr path);

}  // namespace plai
