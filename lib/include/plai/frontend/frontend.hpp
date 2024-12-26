#pragma once

#include <memory>
#include <plai/frontend/events.hpp>
#include <plai/frontend/texture.hpp>
#include <plai/frontend/type.hpp>

namespace plai {

/**
 * \brief Frontend
 *
 * This is what actually displays the window etc.
 * */
class Frontend {
 public:
    virtual ~Frontend() = default;

    /**
     * \brief Poll an event from the frontend
     *
     * Returns an event when possible and std::nullopt if no events are
     * available or the event from the library is not convertible to any
     * plai::Event types.
     *
     * */
    virtual std::optional<Event> poll_event() = 0;

    virtual Vec<int> dimensions() = 0;

    // TODO: make pure virtual
    virtual void render_clear() {}

    /**
     * \brief Create a new texture
     *
     * Creates a new texture with the given render target options. The same
     * texture is used for rendering all the targets.
     * */
    virtual std::unique_ptr<Texture> texture() = 0;

    virtual void render_current() = 0;
};

std::unique_ptr<Frontend> frontend(FrontendType type);
inline std::unique_ptr<Frontend> frontend(std::string_view nm) {
    return frontend(frontend_type(nm));
}

}  // namespace plai
