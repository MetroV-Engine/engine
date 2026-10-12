#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "../Entity.hpp"
#include "../Registry.hpp"
#include "Ref.hpp"
#include "TypeInfo.hpp"
#include "TypeRegistry.hpp"
#include "detail/ComponentCatalog.hpp"

namespace ECS::reflect {
    /**
     * @brief Makes T usable as a component by TypeInfo.
     *
     * Done automatically for every E_REFLECT type; use this for a type that
     * is not reflected, or whose automatic registration was dropped by the
     * linker (see detail::ComponentCatalog).
     * @return T's TypeInfo.
     */
    template<ComponentStorable T>
    const TypeInfo& registerComponentType() {
        return *detail::ComponentCatalog::instance().add<T>().type;
    }

    /**
     * @brief Access to a Registry's components without knowing their types.
     *
     * For code that only has a component's TypeInfo: editor, interpreted
     * scripting, scene loading, serialization, debug console. Code that knows
     * its types (gameplay, C++ scripts) uses the Registry's typed API instead.
     * Both share the same implementation.
     *
     * Obtained with Registry::inspect(). It is a cheap handle referencing the
     * Registry (no state of its own): create it where needed, it must not
     * outlive the Registry.
     *
     * Refs to components are invalidated by any structural change of that
     * component type (add/remove on any entity), like a reference from
     * Registry::getComponent<T>: fetch them again rather than keeping them
     * across frames.
     */
    class RegistryInspector {
        public:

            // --- Component types ----------------------------------------------

            /** @brief TypeInfos of every type usable as a component, e.g. for an "Add component" menu. */
            [[nodiscard]] static std::vector<const TypeInfo*> types() {
                std::vector<const TypeInfo*> types;
                for (const detail::ComponentInfo* info : detail::ComponentCatalog::instance().components()) {
                    types.push_back(info->type);
                }
                return types;
            }

            /**
             * @brief The component type with this name, or nullptr.
             *
             * Unlike TypeRegistry::find, only returns types usable as
             * components (not "float" or "vector<Item>").
             */
            [[nodiscard]] static const TypeInfo* findType(std::string_view name) noexcept {
                const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(typeId(name));
                return info && info->type->name == name ? info->type : nullptr;
            }

            /**
             * @brief Failures of the automatic registration (e.g. two types
             *        reflected under the same name).
             *
             * Registration runs before main, where an exception would abort
             * the program, so failures are recorded instead. Check this list
             * at startup to surface them.
             */
            [[nodiscard]] static const std::vector<std::string>& errors() noexcept {
                return detail::ComponentCatalog::instance().errors();
            }

            // --- Components of an entity ----------------------------------------

            /**
             * @brief Refs to every component of an entity whose type is a component type.
             * @return Ordered by ComponentId; empty for a dead entity. Components
             *         of types that are not registered as components are skipped.
             */
            [[nodiscard]] std::vector<Ref> list(Entity entity) {
                std::vector<Ref> components;
                for (const ComponentId id : _registry.componentIdsOf(entity)) {
                    if (const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(id)) {
                        components.emplace_back(*info->type, _registry.getComponentById(entity, id));
                    }
                }
                return components;
            }

            /**
             * @brief Ref to an entity's component of this type.
             * @return An invalid Ref when absent or when type is not a component type.
             * @throws std::invalid_argument if entity is not alive.
             */
            [[nodiscard]] Ref get(Entity entity, const TypeInfo& type) {
                const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(type);
                if (!info) {
                    return {};
                }
                void* data = _registry.getComponentById(entity, info->id);
                return data ? Ref(type, data) : Ref{};
            }

            /**
             * @brief Checks whether an entity owns the component of this type.
             * @return False for a dead entity or a type that is not a component type.
             */
            [[nodiscard]] bool has(Entity entity, const TypeInfo& type) const {
                const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(type);
                return info && _registry.hasComponentsById(entity, {info->id});
            }

            /**
             * @brief Adds (or replaces) a default-constructed component of this type.
             *
             * Creates the type's pool in the Registry if needed. Under
             * runDeferred, the Ref points to the value applied at flush, like
             * Registry::addComponent<T>.
             * @return An invalid Ref when type is not a component type or has no
             *         default constructor.
             * @throws std::invalid_argument if entity is not alive.
             * @throws std::out_of_range when the type's ComponentId does not fit in a Signature.
             */
            Ref add(Entity entity, const TypeInfo& type) {
                const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(type);
                if (!info) {
                    return {};
                }
                _registry.ensurePoolById(info->id, info->makePool);
                void* data = _registry.addDefaultComponentById(entity, info->id);
                return data ? Ref(type, data) : Ref{};
            }

            /**
             * @brief Removes an entity's component of this type, if any.
             * @throws std::invalid_argument if entity is not alive.
             */
            void remove(Entity entity, const TypeInfo& type) {
                if (const detail::ComponentInfo* info = detail::ComponentCatalog::instance().find(type)) {
                    _registry.removeComponentById(entity, info->id);
                }
            }

        private:
            friend class ECS::Registry;   // only Registry::inspect() creates one

            explicit RegistryInspector(Registry& registry) noexcept : _registry(registry) {}

            Registry& _registry;
    };
}

namespace ECS {
    // Declared in Registry.hpp, defined here so the ECS core does not depend
    // on the reflection layer.
    inline reflect::RegistryInspector Registry::inspect() {
        return reflect::RegistryInspector(*this);
    }
}
