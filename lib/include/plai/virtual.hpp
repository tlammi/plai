#pragma once

namespace plai {

class Virtual {
 public:
    constexpr Virtual() noexcept = default;
    Virtual(const Virtual&) = delete;
    Virtual& operator=(const Virtual&) = delete;
    Virtual(Virtual&&) = delete;
    Virtual& operator=(Virtual&&) = delete;
    virtual ~Virtual() = default;
};
}  // namespace plai
