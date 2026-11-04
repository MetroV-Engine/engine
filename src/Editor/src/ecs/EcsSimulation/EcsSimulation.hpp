/*
    Created on: 03/11/2026
    Filename: EcsSimulation.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#pragma once

#include "scene/Simulation.hpp"

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"

#include <memory>
#include <string>
#include <vector>

// Play mode on an ECS world: update() runs the systems, and Stop puts the
// world back as it was on Play, so playing never changes the edited level.
//
// A Registry cannot be copied, so begin() saves a copy of every reflected
// component of every entity (TypeInfo::copy), and end() restores it in place:
//  - entities spawned during play are killed;
//  - entities still alive keep their handle (the editor's selection stays
//    valid): components added during play are removed, removed ones come
//    back, and every saved value is written back;
//  - entities killed during play are spawned again, with a new handle.
//
// Not saved, so left as they are on Stop: components whose type is not
// reflected or not copyable (a warning lists the latter), resources, and the
// systems' own state. A component holding an entity handle still holds the
// old handle of an entity respawned on Stop.
class EcsSimulation : public Simulation
{
public:
    // world and systems must outlive the simulation.
    EcsSimulation(ECS::Registry &world, ECS::SystemManager &systems) : _world(world), _systems(systems) {}

    void begin() override;
    void update(double dt) override;
    void end() override;

private:
    // Frees a value copied by TypeInfo::copy into storage of its own.
    struct ValueDeleter
    {
        const ECS::reflect::TypeInfo *type;
        void operator()(void *value) const noexcept;
    };
    using SavedValue = std::unique_ptr<void, ValueDeleter>;

    struct SavedEntity
    {
        ECS::Entity entity;
        std::vector<SavedValue> components;
        // Components present on Play but not copyable: kept as they are.
        std::vector<const ECS::reflect::TypeInfo *> unsaved;
    };

    ECS::Registry &_world;
    ECS::SystemManager &_systems;
    // In ascending entity order, like the inspector lists them.
    std::vector<SavedEntity> _saved;

    static SavedValue copyValue(const ECS::reflect::TypeInfo &type, const void *source);
    bool wasSaved(ECS::Entity entity) const;
    // Restores one entity; adds the types it could not restore to failed.
    void restore(SavedEntity &saved, std::vector<std::string> &failed);
};
