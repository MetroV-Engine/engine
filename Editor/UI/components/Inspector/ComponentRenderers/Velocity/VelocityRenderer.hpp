#ifndef PROTO_VELOCITYRENDERER_HPP
#define PROTO_VELOCITYRENDERER_HPP

#include "IComponent.hpp"

namespace Editor::UI::Components::Renderers {
    class VelocityRenderer {
    public:
        static void render(IComponent &component);
    };
}

#endif //PROTO_VELOCITYRENDERER_HPP
