#pragma once

#include <typeindex>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <any>
#include <stdexcept>
#include <vector>
#include <functional>
#include <utility>

#include "SparseSet.hpp"
#include "Entity.hpp"
#include "IComponent.hpp"

namespace ECS {

    // -------------------------------------------------------------------------
    // Type-erased pool interface — lets the registry iterate all pools and
    // retrieve IComponent* for a given entity id without knowing the type.
    // -------------------------------------------------------------------------
    struct IComponentPool {
        virtual ~IComponentPool() = default;
        virtual bool        has(std::size_t entity_id) const = 0;
        virtual IComponent* get(std::size_t entity_id)       = 0;
    };

    template<typename Component>
    struct ComponentPool final : IComponentPool {
        sparse_set<Component>* pool;
        explicit ComponentPool(sparse_set<Component>* p) : pool(p) {}

        bool has(std::size_t entity_id) const override {
            return pool->has(entity_id);
        }
        IComponent* get(std::size_t entity_id) override {
            return &pool->get(entity_id);
        }
    };

}

namespace ECS {
    // Registry that stores one sparse_set<Component> per component type using std::any.
    // Also responsible for managing entities (spawn/kill) and for adding/removing components.
    class registry {
    public:
        registry();
        ~registry();
        registry(registry const&) = delete;
        registry& operator=(registry const&) = delete;

        // Adds a new component array for Component if absent, returns reference.
        // Also registers an "eraser" function that will remove the component when killing an entity.
        template<typename Component>
        sparse_set<Component>& register_component() {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end()) {
                auto [ins_it, ok] = _components_arrays.emplace(key, std::any(sparse_set<Component>{}));
                it = ins_it;

                // register eraser for this component type
                _erasers.emplace_back([this](ECS::Entity::entity const& e) {
                    auto *arr = this->get_if<Component>();
                    if (!arr) return;
                    auto idx = static_cast<typename sparse_set<Component>::size_type>(static_cast<size_t>(e));
                    if (arr->has(idx)) arr->erase(idx);
                });

                // register a type-erased pool accessor for get_components_for()
                auto* pool = std::any_cast<sparse_set<Component>>(&it->second);
                _pools.emplace(key, std::make_unique<ComponentPool<Component>>(pool));
            }
            return std::any_cast<sparse_set<Component>&>(it->second);
        }

        // Retrieve the component array for Component. Throws std::out_of_range if not registered.
        template<typename Component>
        sparse_set<Component>& get_components() {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end())
                throw std::out_of_range("registry::get_components: component not registered");
            return std::any_cast<sparse_set<Component>&>(it->second);
        }

        // Const overload: retrieve const reference to the component array. Throws if not registered.
        template<typename Component>
        sparse_set<Component> const& get_components() const {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end())
                throw std::out_of_range("registry::get_components const: component not registered");
            return std::any_cast<sparse_set<Component> const&>(it->second);
        }

        template<typename Component>
        Component& get_component(ECS::Entity::entity const& e) {
            auto& arr = get_components<Component>();
            using idx_t = typename sparse_set<Component>::size_type;
            idx_t idx = static_cast<idx_t>(static_cast<size_t>(e));
            if (!arr.has(idx)) {
                throw std::out_of_range("registry::get_component: entity does not have component");
            }
            return arr[idx];
        }

        template<typename Component>
        Component const& get_component(ECS::Entity::entity const& e) const {
            auto const& arr = get_components<Component>();
            using idx_t = typename sparse_set<Component>::size_type;
            idx_t idx = static_cast<idx_t>(static_cast<size_t>(e));
            if (!arr.has(idx)) {
                throw std::out_of_range("registry::get_component const: entity does not have component");
            }
            return arr[idx];
        }

        // add_component accepts both lvalue and rvalue (universal reference)
        template<typename Component>
        typename sparse_set<Component>::reference_type
        add_component(ECS::Entity::entity const& to, Component&& c) {
            auto &arr = register_component<Component>();
            using idx_t = typename sparse_set<Component>::size_type;
            return arr.insert_at(static_cast<idx_t>(static_cast<size_t>(to)), std::forward<Component>(c));
        }

        template<typename Component, typename... Params>
        typename sparse_set<Component>::reference_type
        emplace_component(ECS::Entity::entity const& to, Params&&... p) {
            auto &arr = register_component<Component>();
            using idx_t = typename sparse_set<Component>::size_type;
            return arr.emplace_at(static_cast<idx_t>(static_cast<size_t>(to)), std::forward<Params>(p)...);
        }

        template<typename Component>
        void remove_component(ECS::Entity::entity const& from) {
            auto *arr = get_if<Component>();
            if (!arr) return;
            using idx_t = typename sparse_set<Component>::size_type;
            idx_t idx = static_cast<idx_t>(static_cast<size_t>(from));
            if (idx < arr->size()) arr->erase(idx);
        }

        // helper: get_if
        template<typename Component>
        sparse_set<Component>* get_if() noexcept {
            auto key = std::type_index(typeid(Component));
            auto it = _components_arrays.find(key);
            if (it == _components_arrays.end()) return nullptr;
            return std::any_cast<sparse_set<Component>>(&it->second);
        }

        // Systems: register callable systems that will be invoked by run_systems().
        // The callable should accept (registry&, sparse_set<Component>&...) as parameters.
        template<class... Components, typename Function>
        void add_system(Function&& f) {
            using Fn = std::decay_t<Function>;
            _systems.emplace_back([func = Fn(std::forward<Function>(f))](registry &r) mutable {
                func(r, r.get_components<Components>()...);
            });
        }

        template<class... Components, typename Function>
        void add_system(Function const& f) {
            _systems.emplace_back([f](registry &r) {
                f(r, r.get_components<Components>()...);
            });
        }

        void run_systems();
        ECS::Entity::entity spawn_entity();
        ECS::Entity::entity entity_from_index(std::size_t idx) const;
        void kill_entity(ECS::Entity::entity const& e);

        // Get all active entities
        std::vector<ECS::Entity::entity> get_all_entities() const;

        // Entity name management
        void set_entity_name(ECS::Entity::entity const& e, const std::string& name);
        std::string get_entity_name(ECS::Entity::entity const& e) const;

        // Returns only the components that entity_id actually owns.
        // Iterates the _pools unordered_map — O(number of registered component types),
        // but only pushes back what the entity has.
        std::vector<IComponent*> get_components_for(std::size_t entity_id) {
            std::vector<IComponent*> result;
            for (auto& [type, pool] : _pools)
                if (pool->has(entity_id))
                    result.push_back(pool->get(entity_id));
            return result;
        }

    private:
        std::unordered_map<std::type_index, std::any>                        _components_arrays;
        std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> _pools;
        std::vector<std::function<void(ECS::Entity::entity const&)>>         _erasers;
        std::vector<std::function<void(registry&)>>                          _systems;
        std::vector<std::size_t>                                             _free_ids;
        std::size_t                                                          _next_id{0};

        // Storage for entity names
        std::unordered_map<std::size_t, std::string> _entity_names;

        // Cached entities list for performance
        mutable std::vector<ECS::Entity::entity> _cached_entities;
        mutable bool _entities_dirty = true;
    };
}
