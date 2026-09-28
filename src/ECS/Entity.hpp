#pragma once

#include <cstddef>
#include <compare>
#include <cstdint>

namespace ECS {
    using EntityGeneration = std::uint32_t;

    /**
     * @brief Identity of the EntityManager (and so the Registry) that issued a handle.
     *
     * Every EntityManager draws a unique, non-zero WorldId. InvalidWorld is
     * never issued, so a handle carrying it is never alive anywhere.
     */
    using WorldId = std::uint32_t;
    inline constexpr WorldId InvalidWorld = 0;

    class EntityManager;

#ifdef ENGINE_TESTING
    namespace TestAccess {
        struct EntityAccess;
    }
#endif

    /**
     * @brief Lightweight identity used to refer to an entity in the ECS.
     *
     * Entity stores an index, a generation and the WorldId of the manager
     * that issued it. Creation, destruction and validity checks belong to
     * EntityManager, which is the only code allowed to build a non-default
     * handle: a handle only ever comes from the Registry that owns the
     * entity, and is rejected by every other Registry. The identity is
     * cheap to copy and its index can be used as a key by sparse component
     * storage.
     */
    class Entity {
        friend class EntityManager;
#ifdef ENGINE_TESTING
        friend struct TestAccess::EntityAccess;
#endif

        public:
            /**
             * @brief Creates a handle that refers to no entity.
             *
             * It carries InvalidWorld, so it is never alive in any Registry.
             * It exists so handles can be default-initialized (component
             * members, containers, variables assigned later).
             */
            Entity() noexcept = default;

            /**
             * @brief Converts the handle to its numeric identity.
             * @return The identity stored by this handle.
             */
            operator std::size_t() const noexcept { return _index; }

            /**
             * @brief Reads the numeric identity without conversion syntax.
             * @return The identity stored by this handle.
             */
            std::size_t value() const noexcept { return _index; }

            /** @brief Reads the stable slot index stored by this handle. */
            std::size_t index() const noexcept { return _index; }

            /** @brief Reads the slot generation stored by this handle. */
            EntityGeneration generation() const noexcept { return _generation; }

            /** @brief Reads the WorldId of the manager that issued this handle. */
            WorldId world() const noexcept { return _world; }

            /**
             * @brief Compares entity identities using three-way comparison.
             * @param other Handle whose identity is compared with this one.
             * @return Ordering based on the world, then the index, then the
             *         generation.
             */
            std::strong_ordering operator<=>(const Entity& other) const noexcept {
                if (const auto ordering = _world <=> other._world;
                    ordering != 0) {
                    return ordering;
                }
                if (const auto ordering = _index <=> other._index;
                    ordering != 0) {
                    return ordering;
                }
                return _generation <=> other._generation;
            }

            /**
             * @brief Compares every field of two handles.
             *
             * Declared explicitly: without it, `a == b` would convert both
             * handles to std::size_t and compare indexes only.
             */
            bool operator==(const Entity& other) const noexcept = default;

        private:
            Entity(std::size_t index, EntityGeneration generation, WorldId world) noexcept
                : _index(index), _generation(generation), _world(world) {}

            std::size_t _index{0};
            EntityGeneration _generation{0};
            WorldId _world{InvalidWorld};
    };

    static_assert(sizeof(std::size_t) != 8 || sizeof(Entity) == 16,
        "ECS::Entity: the WorldId must fit in the padding after the generation");
}
