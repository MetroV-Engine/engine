#pragma once

#include <cstddef>
#include <compare>
#include <cstdint>

namespace ECS {
    using EntityGeneration = std::uint32_t;

    /**
     * @brief Lightweight identity used to refer to an entity in the ECS.
     *
    * Entity stores an index and a generation.
     * Creation, destruction and validity checks belong to EntityManager. The
     * identity is therefore cheap to copy and can be used as a key by sparse
     * component storage.
     */
    class Entity {
        public:
            /**
             * @brief Creates an entity handle for an existing numeric identity.
             * @param id Numeric identity assigned by the entity manager.
             */
            explicit Entity(std::size_t id) noexcept : _index(id) {}

            /** @brief Creates a handle with an explicit index and generation. */
            Entity(std::size_t index, EntityGeneration generation) noexcept
                : _index(index), _generation(generation) {}

            /**
             * @brief Creates the default handle with identity zero.
             *
             * This constructor is retained for compatibility with the current
             * engine API. A default-constructed handle is not proof that an
             * entity is alive; use EntityManager::isAlive() for that check.
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

            /**
             * @brief Compares entity identities using three-way comparison.
             * @param other Handle whose identity is compared with this one.
             * @return Ordering based on the index and generation.
             */
            std::strong_ordering operator<=>(const Entity& other) const noexcept {
                if (const auto ordering = _index <=> other._index;
                    ordering != 0) {
                    return ordering;
                }
                return _generation <=> other._generation;
            }

        private:
            std::size_t _index{0};
            EntityGeneration _generation{0};
    };
}
     