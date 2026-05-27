/*
    Created on: 3/10/26
    Filename: NativeScriptSystem.cpp
    Description: Calls onStart once and onUpdate every frame for all active
                 NativeScriptComponents. The entity is looked up from the
                 sparse_set index so each script knows which entity it owns.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "NativeScriptSystem.hpp"
#include "Entity/Base/NativeScript/NativeScriptComponent.hpp"

namespace ECS::System {

    void NativeScriptSystem::update(registry &r, float dt, SDL_Window * /*win*/)
    {
        auto *arr = r.get_if<ECS::Entity::NativeScriptComponent>();
        if (!arr) return;

        const auto entities = r.get_all_entities();

        for (auto &entity : entities) {
            using idx_t = sparse_set<ECS::Entity::NativeScriptComponent>::size_type;
            auto idx = static_cast<idx_t>(static_cast<std::size_t>(entity));

            if (!arr->has(idx)) continue;

            auto &script = arr->get(idx);
            if (!script.active) continue;

            // Call onStart exactly once
            if (!script.started) {
                if (script.onStart)
                    script.onStart(r, entity, dt);
                script.started = true;
            }

            // Call onUpdate every frame
            if (script.onUpdate)
                script.onUpdate(r, entity, dt);
        }
    }

} // ECS::System

