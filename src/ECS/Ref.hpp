#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "TypeInfo.hpp"
#include "TypeRegistry.hpp"

namespace ECS::reflect {
    /**
     * @brief Non-owning, type-erased view on a value.
     *
     * Pairs a TypeInfo with the address of an object of that type. It is the
     * handle the editor, scripts and serializers manipulate: fields are reached
     * by name at runtime, and the value is read back with as<T>() once the
     * caller knows (or checks) its type.
     *
     * A Ref does not extend the lifetime of the object it points to.
     */
    class Ref {
        public:
            /** @brief Builds an invalid Ref. */
            Ref() noexcept = default;

            /** @brief Wraps data, which must point to an object of type `type`. */
            Ref(const TypeInfo& type, void* data) noexcept : _type(&type), _data(data) {}

            /** @brief Wraps a typed object, registering T if needed. */
            template<typename T>
            [[nodiscard]] static Ref make(T& obj) {
                return Ref(TypeRegistry::instance().registerType<T>(), &obj);
            }

            /** @brief True when the Ref points to an object. */
            [[nodiscard]] bool valid() const noexcept { return _type && _data; }
            explicit operator bool() const noexcept { return valid(); }

            /** @brief Type of the pointed object; requires valid(). */
            [[nodiscard]] const TypeInfo& type() const noexcept { return *_type; }

            /** @brief Raw address of the pointed object (null if invalid). */
            [[nodiscard]] void* data() const noexcept { return _data; }

            /**
             * @brief Returns a Ref to the named field.
             * @return An invalid Ref if this Ref is invalid or has no such field,
             *         so lookups can be chained: `ref.field("pos").field("x")`.
             */
            [[nodiscard]] Ref field(std::string_view name) const noexcept {
                if (!valid()) {
                    return {};
                }
                const FieldInfo* info = _type->findField(name);
                return info ? field(*info) : Ref{};
            }

            /** @brief Returns a Ref to a field of this Ref's type; requires valid(). */
            [[nodiscard]] Ref field(const FieldInfo& info) const noexcept {
                return Ref(*info.type, info.access(_data));
            }

            /**
             * @brief Follows a dot-separated chain of field names.
             * @param path e.g. "transform.pos.x"; equivalent to chaining field().
             * @return This Ref for an empty path; an invalid Ref if any segment
             *         is missing or empty ("a..b", ".a", "a.").
             */
            [[nodiscard]] Ref path(std::string_view path) const noexcept {
                if (path.empty()) {
                    return *this;
                }
                Ref current = *this;
                while (current) {
                    const std::size_t dot = path.find('.');
                    current = current.field(path.substr(0, dot));
                    if (dot == std::string_view::npos) {
                        break;
                    }
                    path.remove_prefix(dot + 1);
                }
                return current;
            }

            /** @brief Returns the value as T if it is exactly a T, nullptr otherwise. */
            template<typename T>
            [[nodiscard]] T* tryAs() const noexcept {
                if (!valid() || _type != TypeRegistry::instance().find<T>()) {
                    return nullptr;
                }
                return static_cast<T*>(_data);
            }

            /**
             * @brief Returns the value as T.
             * @throws std::logic_error If the Ref is invalid or not a T.
             */
            template<typename T>
            [[nodiscard]] T& as() const {
                if (T* value = tryAs<T>()) {
                    return *value;
                }
                throw std::logic_error("Ref::as: value is " +
                                       (valid() ? "'" + _type->name + "'"
                                                : std::string("an invalid Ref")) +
                                       ", not the requested type");
            }

        private:
            const TypeInfo* _type = nullptr;
            void* _data = nullptr;
    };
}
