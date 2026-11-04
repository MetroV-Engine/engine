/*
    Created on: 03/11/2026
    Filename: EcsSimulation.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "EcsSimulation.hpp"
#include "core/Logger/Logger.hpp"

#include <algorithm>
#include <exception>
#include <new>
#include <set>
#include <utility>

namespace {

namespace reflect = ECS::reflect;

std::string join(const std::vector<std::string> &names)
{
    std::string text;
    for (const std::string &name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

// The pool still owns target and destroys it later, so it must hold a live
// object again. Without a default constructor (or if it throws too, hence
// noexcept) the pool cannot be kept consistent.
void reviveDefault(const reflect::TypeInfo &type, void *target) noexcept
{
    if (!type.construct)
        std::terminate();
    type.construct(target);
}

// Replaces the value at target by a copy of source, in place: TypeInfo has
// no assignment, only destroy and copy-construct.
void overwrite(const reflect::TypeInfo &type, void *target, const void *source)
{
    type.destroy(target);
    try {
        type.copy(target, source);
    } catch (...) {
        reviveDefault(type, target);
        throw;
    }
}

} // namespace

void EcsSimulation::ValueDeleter::operator()(void *value) const noexcept
{
    type->destroy(value);
    ::operator delete(value, std::align_val_t{type->align});
}

EcsSimulation::SavedValue EcsSimulation::copyValue(const reflect::TypeInfo &type, const void *source)
{
    void *storage = ::operator new(type.size, std::align_val_t{type.align});
    try {
        type.copy(storage, source);
    } catch (...) {
        ::operator delete(storage, std::align_val_t{type.align});
        throw;
    }
    return SavedValue(storage, ValueDeleter{&type});
}

void EcsSimulation::begin()
{
    _saved.clear();
    reflect::RegistryInspector inspector = _world.inspect();
    std::set<std::string> unsaved;
    try {
        for (const ECS::Entity entity : inspector.entities()) {
            SavedEntity saved{entity, {}, {}};
            for (const reflect::Ref &component : inspector.list(entity)) {
                if (component.type().copy) {
                    saved.components.push_back(copyValue(component.type(), component.data()));
                } else {
                    saved.unsaved.push_back(&component.type());
                    unsaved.insert(component.type().name);
                }
            }
            _saved.push_back(std::move(saved));
        }
    } catch (...) {
        _saved.clear(); // The game does not start: nothing to restore
        throw;
    }
    if (!unsaved.empty())
        Logger::instance().warning("Game", "Not copyable, so not restored on Stop: "
                + join(std::vector<std::string>(unsaved.begin(), unsaved.end())));
}

void EcsSimulation::update(double dt)
{
    _systems.update(_world, dt);
}

bool EcsSimulation::wasSaved(ECS::Entity entity) const
{
    return std::ranges::binary_search(_saved, entity, {}, &SavedEntity::entity);
}

void EcsSimulation::end()
{
    // Spawned during play. The list iterates over a snapshot, so killing
    // entities meanwhile is safe.
    for (const ECS::Entity entity : _world.inspect().entities())
        if (!wasSaved(entity))
            _world.killEntity(entity);

    std::vector<std::string> failed;
    for (SavedEntity &saved : _saved)
        restore(saved, failed);
    _saved.clear();
    if (!failed.empty())
        Logger::instance().warning("Game", "Not restored on Stop: " + join(failed));
}

void EcsSimulation::restore(SavedEntity &saved, std::vector<std::string> &failed)
{
    reflect::RegistryInspector inspector = _world.inspect();
    const bool alive = _world.isAlive(saved.entity);
    // Killed during play: a handle cannot be revived, the entity comes back
    // as a new one.
    const ECS::Entity entity = alive ? saved.entity : _world.spawnEntity();
    if (!alive)
        for (const reflect::TypeInfo *type : saved.unsaved)
            failed.push_back(type->name);

    // Added during play. Types first: removing a component invalidates the Refs.
    const auto wasPresent = [&saved](const reflect::TypeInfo *type) {
        return std::ranges::find(saved.unsaved, type) != saved.unsaved.end()
            || std::ranges::any_of(saved.components, [type](const SavedValue &value) { return value.get_deleter().type == type; });
    };
    std::vector<const reflect::TypeInfo *> current;
    for (const reflect::Ref &component : inspector.list(entity))
        current.push_back(&component.type());
    for (const reflect::TypeInfo *type : current)
        if (!wasPresent(type))
            inspector.remove(entity, *type);

    for (const SavedValue &value : saved.components) {
        const reflect::TypeInfo &type = *value.get_deleter().type;
        try {
            reflect::Ref target = inspector.get(entity, type);
            if (!target)
                target = inspector.add(entity, type); // Removed during play
            if (!target) {
                failed.push_back(type.name + " (no default constructor)");
                continue;
            }
            overwrite(type, target.data(), value.get());
        } catch (const std::exception &e) {
            failed.push_back(type.name + " (" + e.what() + ")");
        }
    }
}
