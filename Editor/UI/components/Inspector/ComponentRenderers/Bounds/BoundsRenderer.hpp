#ifndef PROTO_BOUNDSRENDERER_HPP
#define PROTO_BOUNDSRENDERER_HPP

#include "IComponent.hpp"

namespace Editor::UI::Components::Renderers {
    class BoundsRenderer {
    public:
        static void render(IComponent &component, bool &preserveRatio, float &storedRatio);
    };
}

#endif //PROTO_BOUNDSRENDERER_HPP
