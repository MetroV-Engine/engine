/*
** EPITECH PROJECT, 2025
** asd
** File description:
** asd
*/

#include "../Include/Registry.hpp"

#include <algorithm>
#include <set>

namespace ECS {
    registry::registry() = default;
    registry::~registry() = default;

    ECS::Entity::entity registry::spawn_entity() {
        _entities_dirty = true; // Mark cache as dirty
        if (!_free_ids.empty()) {
            auto id = _free_ids.back();
            _free_ids.pop_back();
            return ECS::Entity::entity(id);
        }
        return ECS::Entity::entity(_next_id++);
    }

    ECS::Entity::entity registry::entity_from_index(std::size_t idx) const {
        return ECS::Entity::entity(idx);
    }

    void registry::kill_entity(ECS::Entity::entity const& e) {
        for (auto &fn : _erasers) fn(e);
        _free_ids.push_back(static_cast<size_t>(e));
        // Clean up entity name when killing entity
        _entity_names.erase(static_cast<size_t>(e));
        _entities_dirty = true; // Mark cache as dirty
    }

    void registry::run_systems() {
        for (auto &s : _systems) s(*this);
    }

    std::vector<ECS::Entity::entity> registry::get_all_entities() const {
        // Return cached entities if not dirty
        if (!_entities_dirty) {
            return _cached_entities;
        }

        // Rebuild the cached entities list
        _cached_entities.clear();

        // Create a set of free IDs for O(log n) lookup
        std::set<std::size_t> free_set(_free_ids.begin(), _free_ids.end());

        // Add all entity IDs from 0 to _next_id-1 that are not in free_ids
        for (std::size_t i = 0; i < _next_id; ++i) {
            if (free_set.find(i) == free_set.end()) {
                ECS::Entity::entity entity(i);
                // Set the name from registry storage
                auto name_it = _entity_names.find(i);
                if (name_it != _entity_names.end()) {
                    entity.setName(name_it->second);
                }
                _cached_entities.emplace_back(entity);
            }
        }

        _entities_dirty = false; // Mark cache as clean
        return _cached_entities;
    }

    void registry::set_entity_name(ECS::Entity::entity const& e, const std::string& name) {
        _entity_names[static_cast<size_t>(e)] = name;
        _entities_dirty = true; // Mark cache as dirty since names changed
    }

    std::string registry::get_entity_name(ECS::Entity::entity const& e) const {
        auto it = _entity_names.find(static_cast<size_t>(e));
        return (it != _entity_names.end()) ? it->second : "";
    }
}