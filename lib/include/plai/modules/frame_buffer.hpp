#pragma once

#include <plai/media/frame.hpp>
#include <plai/virtual.hpp>

namespace plai::mod {

class FrameBuffer : public Virtual {
 public:
    /**
     * \brief Get a frame for modification
     *
     * The received frame can be modified by the caller and then pushed
     * back to the buffer with \a push_frame().
     * */
    virtual media::Frame get_frame() = 0;

    /**
     * \brief Push a new frame to the buffer
     * */
    virtual void push_frame(media::Frame frm) = 0;
};

}  // namespace plai::mod
