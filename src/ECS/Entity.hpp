#pragma once

#include <cstddef>
#include <compare>

namespace ECS {
    /**
     * @brief Lightweight identity used to refer to an entity in the ECS.
     *
    * Entity deliberately stores only a numeric identity.
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
            explicit Entity(std::size_t id) noexcept : _id(id) {}

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
            operator std::size_t() const noexcept { return _id; }

            /**
             * @brief Reads the numeric identity without conversion syntax.
             * @return The identity stored by this handle.
             */
            std::size_t value() const noexcept { return _id; }

            /**
             * @brief Compares entity identities using three-way comparison.
             * @param other Handle whose identity is compared with this one.
             * @return Ordering based only on the numeric identity.
             */
            std::strong_ordering operator<=>(const Entity& other) const noexcept {
                return _id <=> other._id;
            }

        private:
            std::size_t _id{0};
    };
}
     