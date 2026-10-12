#pragma once

#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "ComponentId.hpp"
#include "ComponentPool.hpp"
#include "Descriptor.hpp"
#include "Ref.hpp"
#include "Registry.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistry.hpp"

namespace ECS::reflect {
    /** @brief What the runtime needs to handle one component type from its TypeInfo. */
    struct ComponentInfo {
        const TypeInfo* type = nullptr;
        ComponentId id = 0;
        /** @brief Creates the type's pool in a Registry that has none yet. */
        PoolFactory makePool = nullptr;
    };

    /**
     * @brief Process-wide list of the types usable as components by name.
     *
     * In this ECS a component is any data struct attached to an entity, so
     * every E_REFLECT type is added automatically, before main, provided it
     * can be stored in a pool (see ComponentStorable). Other types can be
     * added with registerComponent<T>().
     *
     * Entries are keyed by TypeId (the stable name hash) rather than by
     * address, so lookups keep working if a module is reloaded.
     *
     * Automatic registration relies on the static initialisation of the
     * translation units that contain an E_REFLECT. A translation unit left out
     * by the linker (an unreferenced object file of a static library) never
     * registers its types: link such a library with WHOLE_ARCHIVE, or call
     * registerComponent<T>() explicitly.
     */
    class ComponentCatalog {
        public:
            ComponentCatalog(const ComponentCatalog&) = delete;
            ComponentCatalog& operator=(const ComponentCatalog&) = delete;

            /** @brief Returns the process-wide catalog. */
            [[nodiscard]] static ComponentCatalog& instance() {
                static ComponentCatalog catalog;
                return catalog;
            }

            /**
             * @brief Adds T (registering its TypeInfo) if needed.
             * @throws std::logic_error If another type already uses T's name.
             */
            template<typename T>
                requires std::is_move_constructible_v<T> && std::is_move_assignable_v<T>
            const ComponentInfo& add() {
                const TypeInfo& type = TypeRegistry::instance().registerType<T>();
                if (const ComponentInfo* existing = find(type.id)) {
                    return *existing;
                }
                auto info = std::make_unique<ComponentInfo>(
                    ComponentInfo{&type, componentId<T>(), &ComponentPool<T>::create});
                ComponentInfo& ref = *info;
                _byComponentId.emplace(ref.id, &ref);
                _ordered.push_back(&ref);
                _byTypeId.emplace(type.id, std::move(info));
                return ref;
            }

            /** @brief The component registered under this TypeId, or nullptr. */
            [[nodiscard]] const ComponentInfo* find(TypeId id) const noexcept {
                const auto it = _byTypeId.find(id);
                return it == _byTypeId.end() ? nullptr : it->second.get();
            }

            /** @brief The component registered under this ComponentId, or nullptr. */
            [[nodiscard]] const ComponentInfo* find(ComponentId id) const noexcept {
                const auto it = _byComponentId.find(id);
                return it == _byComponentId.end() ? nullptr : it->second;
            }

            /** @brief Every registered component, in registration order. */
            [[nodiscard]] const std::vector<const ComponentInfo*>& components() const noexcept {
                return _ordered;
            }

            /**
             * @brief Errors raised by automatic registration (e.g. two types
             *        reflected under the same name).
             *
             * Registration runs before main, where an exception would abort
             * the program, so failures are recorded here instead. Check this
             * list at startup to surface them.
             */
            [[nodiscard]] const std::vector<std::string>& registrationErrors() const noexcept {
                return _errors;
            }

            /** @brief Records an automatic registration failure. */
            void recordError(std::string_view typeName, std::string_view what) {
                _errors.push_back(std::string(typeName) + ": " + std::string(what));
            }

        private:
            ComponentCatalog() = default;

            std::unordered_map<TypeId, std::unique_ptr<ComponentInfo>> _byTypeId;
            std::unordered_map<ComponentId, const ComponentInfo*> _byComponentId;
            std::vector<const ComponentInfo*> _ordered;
            std::vector<std::string> _errors;
    };

    /**
     * @brief What a type needs to live in a component pool: the dense
     *        storage moves components around (insertion, swap-and-pop).
     */
    template<typename T>
    concept ComponentStorable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

    namespace detail {
        // Every E_REFLECT type is a component, except those that cannot be
        // stored in a pool (e.g. a const member): those are only reflected.
        template<typename T>
        void registerReflectedType() noexcept {
            try {
                if constexpr (ComponentStorable<T>) {
                    ComponentCatalog::instance().add<T>();
                } else {
                    TypeRegistry::instance().registerType<T>();
                }
            } catch (const std::exception& error) {
                ComponentCatalog::instance().recordError(Descriptor<T>::name, error.what());
            } catch (...) {
                ComponentCatalog::instance().recordError(Descriptor<T>::name, "unknown error");
            }
        }

        // Catalog entry for type, checking it is the very same TypeInfo.
        [[nodiscard]] inline const ComponentInfo* componentInfo(const TypeInfo& type) noexcept {
            const ComponentInfo* info = ComponentCatalog::instance().find(type.id);
            return info && info->type == &type ? info : nullptr;
        }
    }

    /**
     * @brief Makes T usable as a component by name; done automatically for E_REFLECT types.
     * @return T's TypeInfo.
     */
    template<ComponentStorable T>
    const TypeInfo& registerComponent() {
        return *ComponentCatalog::instance().add<T>().type;
    }

    /** @brief TypeInfos of every type usable as a component, e.g. for an "Add component" menu. */
    [[nodiscard]] inline std::vector<const TypeInfo*> componentTypes() {
        std::vector<const TypeInfo*> types;
        for (const ComponentInfo* info : ComponentCatalog::instance().components()) {
            types.push_back(info->type);
        }
        return types;
    }

    /**
     * @brief Checks whether an entity owns the component of this type.
     * @return False for a dead entity or a type that is not a component.
     */
    [[nodiscard]] inline bool hasComponent(const Registry& registry, Entity entity, const TypeInfo& type) {
        const ComponentInfo* info = detail::componentInfo(type);
        return info && registry.hasComponents(entity, {info->id});
    }

    /**
     * @brief Ref to an entity's component of this type.
     * @return An invalid Ref when absent or when type is not a component.
     * @throws std::invalid_argument if entity is not alive.
     *
     * Like a reference from getComponent<T>, the Ref is invalidated by any
     * structural change of that component type (add/remove on any entity):
     * fetch it again rather than keeping it across frames.
     */
    [[nodiscard]] inline Ref getComponent(Registry& registry, Entity entity, const TypeInfo& type) {
        const ComponentInfo* info = detail::componentInfo(type);
        if (!info) {
            return {};
        }
        void* data = registry.getComponent(entity, info->id);
        return data ? Ref(type, data) : Ref{};
    }

    /**
     * @brief Adds (or replaces) a default-constructed component of this type.
     *
     * Creates the type's pool in this registry if needed. Under runDeferred,
     * the Ref points to the value applied at flush, like addComponent<T>.
     * @return An invalid Ref when type is not a component or has no default constructor.
     * @throws std::invalid_argument if entity is not alive.
     * @throws std::out_of_range when the type's ComponentId does not fit in a Signature.
     */
    inline Ref addComponent(Registry& registry, Entity entity, const TypeInfo& type) {
        const ComponentInfo* info = detail::componentInfo(type);
        if (!info) {
            return {};
        }
        registry.ensurePool(info->id, info->makePool);
        void* data = registry.addDefaultComponent(entity, info->id);
        return data ? Ref(type, data) : Ref{};
    }

    /**
     * @brief Removes an entity's component of this type, if any.
     * @throws std::invalid_argument if entity is not alive.
     */
    inline void removeComponent(Registry& registry, Entity entity, const TypeInfo& type) {
        if (const ComponentInfo* info = detail::componentInfo(type)) {
            registry.removeComponent(entity, info->id);
        }
    }

    /**
     * @brief Refs to every component of an entity whose type is in the catalog.
     * @return Ordered by ComponentId; empty for a dead entity. Components of
     *         types that are not in the catalog are skipped.
     */
    [[nodiscard]] inline std::vector<Ref> componentsOf(Registry& registry, Entity entity) {
        std::vector<Ref> components;
        for (const ComponentId id : registry.componentIdsOf(entity)) {
            if (const ComponentInfo* info = ComponentCatalog::instance().find(id)) {
                components.emplace_back(*info->type, registry.getComponent(entity, id));
            }
        }
        return components;
    }
}
