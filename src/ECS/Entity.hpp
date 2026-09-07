#pragma once

#include <cstddef>
#include <compare>
#include <type_traits>
#include <string>

namespace ECS {
    class Entity {
        public:
            // construction must be explicit (no implicit conversion from size_t)
            explicit Entity(std::size_t id) noexcept : id_(id) {}

            // optionally allow default construction (identity 0). Remove if you want to force explicit init.
            Entity() noexcept = default;

            // implicitly convertible to size_t
            operator std::size_t() const noexcept { return id_; }

            // explicit accessor
            std::size_t value() const noexcept { return id_; }

            // Name is added to be able to display the entity in the UI only
            void setName(const std::string &name) { _name = name; };
            [[nodiscard]] const std::string& getName() const { return _name; }

            // Comparisons use the identity only; the name is display metadata.
            std::strong_ordering operator<=>(const Entity& other) const noexcept {
                return id_ <=> other.id_;
            }

        private:
            std::size_t id_{0};
            std::string _name="";
    };
}
     