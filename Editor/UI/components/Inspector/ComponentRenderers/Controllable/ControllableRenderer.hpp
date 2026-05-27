#ifndef PROTO_CONTROLLABLERENDERER_HPP
#define PROTO_CONTROLLABLERENDERER_HPP

#include "IComponent.hpp"
#include "Entity/Base/Controlable/Controlable.hpp"
#include <map>

namespace Editor::UI::Components::Renderers {
    class ControllableRenderer {
    public:
        static void render(IComponent &component);
    private:
        static void renderKeyMapLayout(std::map<ECS::Entity::Moves, Proto::Key> *value);
    };
}

#endif //PROTO_CONTROLLABLERENDERER_HPP
