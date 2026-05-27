/*
    Created on: 06/03/2026
    Filename: SceneSnapshot.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "SceneSnapshot.hpp"

namespace ECS::Reflection {

    // -------------------------------------------------------------------------
    // FieldKey
    // -------------------------------------------------------------------------
    bool SceneSnapshot::FieldKey::operator==(const FieldKey &o) const noexcept
    {
        return entityId      == o.entityId
            && componentName == o.componentName
            && fieldName     == o.fieldName;
    }

    // -------------------------------------------------------------------------
    // FieldKeyHash
    // -------------------------------------------------------------------------
    std::size_t SceneSnapshot::FieldKeyHash::operator()(const FieldKey &k) const noexcept
    {
        std::size_t h = std::hash<std::size_t>{}(k.entityId);
        h ^= std::hash<std::string>{}(k.componentName) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::string>{}(k.fieldName)     + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    // -------------------------------------------------------------------------
    // capture() — walk every entity/component/field and copy values into _data.
    // -------------------------------------------------------------------------
    void SceneSnapshot::capture(ECS::registry &reg)
    {
        _data.clear();

        for (const auto &entity : reg.get_all_entities()) {
            const auto id = static_cast<std::size_t>(entity);

            for (auto *comp : reg.get_components_for(id)) {
                for (const auto &fd : comp->getFields()) {
                    _data[{ id, comp->getName(), fd.name }] = snapshotValue(fd);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // restore() — write saved copies back through the raw FieldDescriptor pointers.
    // -------------------------------------------------------------------------
    void SceneSnapshot::restore(ECS::registry &reg)
    {
        for (const auto &entity : reg.get_all_entities()) {
            const auto id = static_cast<std::size_t>(entity);

            for (auto *comp : reg.get_components_for(id)) {
                for (const auto &fd : comp->getFields()) {
                    const auto it = _data.find({ id, comp->getName(), fd.name });
                    if (it != _data.end())
                        restoreValue(fd, it->second);
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // snapshotValue() — dereference the raw pointer and return a value copy.
    // -------------------------------------------------------------------------
    FieldSnapshot SceneSnapshot::snapshotValue(const FieldDescriptor &fd)
    {
        return std::visit([](auto *ptr) -> FieldSnapshot {
            return *ptr;
        }, fd.valuePtr);
    }

    // -------------------------------------------------------------------------
    // restoreValue() — write the stored copy back through the raw pointer.
    // -------------------------------------------------------------------------
    void SceneSnapshot::restoreValue(const FieldDescriptor &fd, const FieldSnapshot &snap)
    {
        std::visit([&snap](auto *ptr) {
            using T = std::remove_pointer_t<decltype(ptr)>;
            if (const auto *val = std::get_if<T>(&snap))
                *ptr = *val;
        }, fd.valuePtr);
    }

} // namespace ECS::Reflection
