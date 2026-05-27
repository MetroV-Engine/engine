#ifndef PROTO_EDITCOMPONENTSPOPUP_HPP
#define PROTO_EDITCOMPONENTSPOPUP_HPP

#include <set>
#include <string>

namespace ECS {
    class registry;
    namespace Entity {
        class entity;
    }
}

namespace Editor::UI::Components::Popups {
    class EditComponentsPopup {
    public:
        static bool render(
            bool &showPopup,
            ECS::Entity::entity entity,
            ECS::registry &registry,
            std::set<std::string> &selectedComponents,
            bool isEditing = true
        );
    };
}

#endif //PROTO_EDITCOMPONENTSPOPUP_HPP
