#pragma once

#include <plai/c_str.hpp>
#include <plai/media/media.hpp>
#include <plai/virtual.hpp>

namespace plai::ctx {
class MediaStore : public Virtual {
 public:
    virtual void set(CStr nm, std::span<const std::byte> data) = 0;
    virtual bool erase(CStr nm) = 0;
    virtual bool contains(CStr nm) = 0;
    virtual media::Media get(CStr nm) = 0;
};

std::unique_ptr<MediaStore> simple_media_store();

}  // namespace plai::ctx
