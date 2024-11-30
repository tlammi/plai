#pragma once

#include <memory>
#include <plai/modules/frame_buffer.hpp>
#include <plai/vec.hpp>
#include <plai/virtual.hpp>
#include <vector>

namespace plai::mod {

class Decoder : public Virtual {
 public:
    /**
     * \brief Set output frame dimensions
     *
     * Request output frame dimensions. This does need to take effect
     * immediately. E.g. buffering might cause delays
     * */
    virtual void set_dimensions(Vec<int> dims) = 0;

    /**
     * \brief Decode media
     * */
    virtual void decode(std::vector<uint8_t> media) = 0;
};

/**
 * \brief Create a decoder
 * */
std::unique_ptr<Decoder> make_decoder(FrameBuffer* frm_buf);

}  // namespace plai::mod
