#ifndef PROTO_SCRIPTRENDERER_HPP
#define PROTO_SCRIPTRENDERER_HPP

#include <imgui.h>
#include "Entity/Base/Script/ScriptComponent.hpp"
#include "imfilebrowser.h"

namespace Editor::UI::Components::Renderers {
    class ScriptRenderer {
    public:
        static void render(ECS::Entity::ScriptComponent &script, ImGui::FileBrowser &fileBrowser);
    };
}

#endif //PROTO_SCRIPTRENDERER_HPP
