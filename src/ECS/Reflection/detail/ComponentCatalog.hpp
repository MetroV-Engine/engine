#pragma once

#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../../ComponentId.hpp"
#include "../../ComponentPool.hpp"
#include "../Descriptor.hpp"
#include "../TypeInfo.hpp"
#include "../TypeRegistry.hpp"

namespace ECS::reflect {
    /**
     * @brief What a type needs to live in a component pool: the dense
     *        storage moves components around (insertion, swap-and-pop).
     */
    template<typename T>
    concept ComponentStorable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;
}

/**
 * @file
 * @brief Internal: the list of types usable as components by TypeInfo.
 *
 * Not meant to be included directly: use ECS/Reflection/Reflection.hpp, and
 * reach the catalog through RegistryInspector.
 */
namespace ECS::reflect::detail {
    /** @brief What the runtime needs to handle one component type from its TypeInfo. */
    struct ComponentInfo {
        const TypeInfo* type = nullptr;
        ComponentId id = 0;
        /** @brief Creates the type's pool in a Registry that has none yet. */
        PoolFactory makePool = nullptr;
    };

    /**
     * @brief Process-wide list of the types usable as components by TypeInfo.
     *
     * In this ECS a component is any data struct attached to an entity, so
     * every E_REFLECT type is added automatically, before main, provided it
     * can be stored in a pool (see ComponentStorable). Other types are added
     * with registerComponentType<T>().
     *
     * Entries are keyed by TypeId (the stable name hash) rather than by
     * address, so lookups keep working if a module is reloaded.
     *
     * Automatic registration relies on the static initialisation of the
     * translation units that contain an E_REFLECT. A translation unit left out
     * by the linker (an unreferenced object file of a static library) never
     * registers its types: link such a library with WHOLE_ARCHIVE, or call
     * registerComponentType<T>() explicitly.
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
            template<ComponentStorable T>
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

            /** @brief The entry of type, checking it is the very same TypeInfo; nullptr if not a component. */
            [[nodiscard]] const ComponentInfo* find(const TypeInfo& type) const noexcept {
                const ComponentInfo* info = find(type.id);
                return info && info->type == &type ? info : nullptr;
            }

            /** @brief Every registered component, in registration order. */
            [[nodiscard]] const std::vector<const ComponentInfo*>& components() const noexcept {
                return _ordered;
            }

            /** @brief Failures of the automatic registration (see RegistryInspector::errors). */
            [[nodiscard]] const std::vector<std::string>& errors() const noexcept {
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

    // Called by every E_REFLECT before main (declared in Descriptor.hpp).
    // Every reflected type is a component, except those that cannot be stored
    // in a pool (e.g. a const member): those are only reflected. Exceptions
    // cannot leave: they would abort the program before main.
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
}
