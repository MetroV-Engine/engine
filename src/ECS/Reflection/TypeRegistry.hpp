#pragma once

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "ContainerTraits.hpp"
#include "Descriptor.hpp"
#include "TypeInfo.hpp"

namespace ECS::reflect {
    /**
     * @brief Process-wide catalogue of runtime type descriptions.
     *
     * Built-in scalars are registered on construction: bool, char, every
     * fundamental integer type, wchar_t, char8_t, char16_t, char32_t, float,
     * double, long double and std::string. Integers are named by signedness
     * and width ("int32", "uint64"), not by spelling: integer types with the
     * same name, size and alignment share one TypeInfo (aliases), so where
     * std::size_t is unsigned long and std::uint64_t unsigned long long,
     * find<std::size_t>() == find<std::uint64_t>(). Any other type is
     * registered on demand by registerType<T>(), which also registers the
     * types of its fields, recursively:
     *  - E_REFLECT types become TypeKind::Struct with their fields;
     *  - types with a ContainerTraits specialisation become Sequence, Map or
     *    Optional, named after their elements ("vector<float>");
     *  - enums become TypeKind::Enum;
     *  - anything else becomes TypeKind::Opaque.
     * Enum and Opaque names come from typeid(T).name(), which is
     * compiler-specific: do not persist the id of such a type.
     *
     * A registration that throws (name collision) is rolled back entirely:
     * the type and every type registered on its behalf are removed.
     *
     * Registration is not thread-safe; do it at startup, before systems run.
     */
    class TypeRegistry {
        public:
            TypeRegistry(const TypeRegistry&) = delete;
            TypeRegistry& operator=(const TypeRegistry&) = delete;

            /** @brief Returns the process-wide registry. */
            [[nodiscard]] static TypeRegistry& instance() {
                static TypeRegistry registry;
                return registry;
            }

            /**
             * @brief Registers T (and its field types) if needed.
             * @return The TypeInfo of T; stable for the lifetime of the process.
             * @throws std::logic_error If another type already uses the same name.
             */
            template<typename T>
            const TypeInfo& registerType() {
                using U = std::remove_cv_t<T>;
                static_assert(!std::is_reference_v<U>, "references cannot be registered");
                static_assert(!std::is_array_v<U>, "C arrays are not supported, use std::array");

                if (const TypeInfo* existing = find<U>()) {
                    return *existing;
                }
                if constexpr (Reflected<U>) {
                    // Inserted before its fields so self-referencing types
                    // (struct Node { std::vector<Node> children; }) terminate.
                    // If a field fails, everything registered since the mark
                    // goes too: field types such as vector<Node> point to Node.
                    const std::size_t mark = _storage.size();
                    TypeInfo& info = insert<U>(std::string(Descriptor<U>::name), TypeKind::Struct);
                    try {
                        FieldBuilder<U> builder{*this, info.fields};
                        forEachField<U>(builder);
                    } catch (...) {
                        rollback(mark);
                        throw;
                    }
                    return info;
                } else if constexpr (HasContainerTraits<U>) {
                    return registerContainer<U>();
                } else if constexpr (std::is_enum_v<U>) {
                    return registerEnum<U>();
                } else {
                    return insert<U>(typeid(U).name(), TypeKind::Opaque);
                }
            }

            /** @brief Same as registerType<T>(). */
            template<typename T>
            const TypeInfo& get() { return registerType<T>(); }

            /** @brief Returns T's TypeInfo if registered, without registering it. */
            template<typename T>
            [[nodiscard]] const TypeInfo* find() const noexcept {
                const auto it = _byType.find(std::type_index(typeid(std::remove_cv_t<T>)));
                return it == _byType.end() ? nullptr : it->second;
            }

            /** @brief Returns the type registered under this id, or nullptr. */
            [[nodiscard]] const TypeInfo* find(TypeId id) const noexcept {
                const auto it = _byId.find(id);
                return it == _byId.end() ? nullptr : it->second;
            }

            /** @brief Returns the type registered under this name, or nullptr. */
            [[nodiscard]] const TypeInfo* find(std::string_view name) const noexcept {
                const TypeInfo* info = find(typeId(name));
                return info && info->name == name ? info : nullptr;
            }

            /** @brief Every registered type, in registration order. */
            [[nodiscard]] const std::vector<const TypeInfo*>& types() const noexcept {
                return _ordered;
            }

        private:
            TypeRegistry() {
                registerScalar<bool>("bool");
                registerScalar<char>("char");
                // The <cstdint> types first, in the historical order: they own
                // the TypeInfo of their name; same-width fundamental types alias it.
                registerInteger<std::int8_t>();
                registerInteger<std::int16_t>();
                registerInteger<std::int32_t>();
                registerInteger<std::int64_t>();
                registerInteger<std::uint8_t>();
                registerInteger<std::uint16_t>();
                registerInteger<std::uint32_t>();
                registerInteger<std::uint64_t>();
                registerScalar<float>("float");
                registerScalar<double>("double");
                registerScalar<std::string>("string");
                registerInteger<signed char>();
                registerInteger<short>();
                registerInteger<int>();
                registerInteger<long>();
                registerInteger<long long>();
                registerInteger<unsigned char>();
                registerInteger<unsigned short>();
                registerInteger<unsigned int>();
                registerInteger<unsigned long>();
                registerInteger<unsigned long long>();
                registerScalar<long double>("long double");
                registerScalar<wchar_t>("wchar");
                registerScalar<char8_t>("char8");
                registerScalar<char16_t>("char16");
                registerScalar<char32_t>("char32");
            }

            // Character types other than char: operator<< is deleted for them
            // on a narrow stream, so they print their numeric code unit.
            template<typename T>
            static constexpr bool isWideCharacter =
                std::is_same_v<T, wchar_t> || std::is_same_v<T, char8_t> ||
                std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

            // Visitor turning each E_FIELD into a FieldInfo with a captureless accessor.
            template<typename T>
            struct FieldBuilder {
                TypeRegistry& registry;
                std::vector<FieldInfo>& out;

                template<auto Member>
                void member(std::string_view name) {
                    using F = typename detail::MemberPointerTraits<decltype(Member)>::Field;
                    const TypeInfo& fieldType = registry.registerType<F>();
                    out.push_back(FieldInfo{
                        std::string(name),
                        &fieldType,
                        [](void* obj) noexcept -> void* {
                            // const_cast for const members: readOnly tells callers not to write.
                            return const_cast<void*>(
                                static_cast<const void*>(&(static_cast<T*>(obj)->*Member)));
                        },
                        std::is_const_v<F>,
                    });
                }
            };

            // Container names are built from their element names, so the
            // element types are registered first. Registering them may have
            // registered T itself (Node -> vector<Node> -> Node), hence the
            // second lookup before inserting.
            template<typename T>
            const TypeInfo& registerContainer() {
                using Traits = ContainerTraits<T>;

                if constexpr (Traits::kind == TypeKind::Sequence) {
                    const TypeInfo& element = registerType<typename Traits::Element>();
                    if (const TypeInfo* existing = find<T>()) {
                        return *existing;
                    }
                    TypeInfo& info = insert<T>(Traits::name(element.name), TypeKind::Sequence);
                    SequenceOps ops;
                    ops.element = &element;
                    ops.size = [](const void* c) noexcept {
                        return Traits::size(*static_cast<const T*>(c));
                    };
                    ops.at = [](void* c, std::size_t index) noexcept -> void* {
                        return Traits::at(*static_cast<T*>(c), index);
                    };
                    if constexpr (requires(T& c) { Traits::resize(c, std::size_t{}); }) {
                        ops.resize = [](void* c, std::size_t count) {
                            Traits::resize(*static_cast<T*>(c), count);
                        };
                    }
                    if constexpr (requires(T& c) { Traits::erase(c, std::size_t{}); }) {
                        ops.erase = [](void* c, std::size_t index) {
                            Traits::erase(*static_cast<T*>(c), index);
                        };
                    }
                    info.sequenceOps = ops;
                    return info;
                } else if constexpr (Traits::kind == TypeKind::Map) {
                    using Key = typename Traits::Key;
                    const TypeInfo& key = registerType<Key>();
                    const TypeInfo& value = registerType<typename Traits::Value>();
                    if (const TypeInfo* existing = find<T>()) {
                        return *existing;
                    }
                    TypeInfo& info = insert<T>(Traits::name(key.name, value.name), TypeKind::Map);
                    MapOps ops;
                    ops.key = &key;
                    ops.value = &value;
                    ops.size = [](const void* m) noexcept {
                        return Traits::size(*static_cast<const T*>(m));
                    };
                    ops.find = [](void* m, const void* k) -> void* {
                        return Traits::find(*static_cast<T*>(m), *static_cast<const Key*>(k));
                    };
                    if constexpr (requires(T& m, const Key& k) { Traits::insert(m, k); }) {
                        ops.insert = [](void* m, const void* k) -> void* {
                            return Traits::insert(*static_cast<T*>(m), *static_cast<const Key*>(k));
                        };
                    }
                    ops.erase = [](void* m, const void* k) {
                        return Traits::erase(*static_cast<T*>(m), *static_cast<const Key*>(k));
                    };
                    ops.forEach = [](void* m, void* ctx,
                                     void (*fn)(void*, const void*, void*)) {
                        Traits::forEach(*static_cast<T*>(m), [&](const auto& k, auto& v) {
                            fn(ctx, &k, &v);
                        });
                    };
                    info.mapOps = ops;
                    return info;
                } else if constexpr (Traits::kind == TypeKind::Optional) {
                    const TypeInfo& value = registerType<typename Traits::Value>();
                    if (const TypeInfo* existing = find<T>()) {
                        return *existing;
                    }
                    TypeInfo& info = insert<T>(Traits::name(value.name), TypeKind::Optional);
                    OptionalOps ops;
                    ops.value = &value;
                    ops.hasValue = [](const void* o) noexcept {
                        return Traits::hasValue(*static_cast<const T*>(o));
                    };
                    ops.get = [](void* o) noexcept -> void* {
                        return Traits::get(*static_cast<T*>(o));
                    };
                    if constexpr (requires(T& o) { Traits::emplace(o); }) {
                        ops.emplace = [](void* o) -> void* {
                            return Traits::emplace(*static_cast<T*>(o));
                        };
                    }
                    ops.reset = [](void* o) noexcept { Traits::reset(*static_cast<T*>(o)); };
                    info.optionalOps = ops;
                    return info;
                } else {
                    static_assert(sizeof(T) == 0, "ContainerTraits<T>::kind must be Sequence, Map or Optional");
                }
            }

            // Enums are exposed through their integer value. Their name comes
            // from typeid (compiler-specific) until enum reflection exists.
            template<typename T>
            const TypeInfo& registerEnum() {
                const TypeInfo& underlying = registerType<std::underlying_type_t<T>>();
                TypeInfo& info = insert<T>(typeid(T).name(), TypeKind::Enum);
                EnumOps ops;
                ops.underlying = &underlying;
                ops.get = [](const void* e) noexcept {
                    return static_cast<std::int64_t>(*static_cast<const T*>(e));
                };
                ops.set = [](void* e, std::int64_t raw) noexcept {
                    *static_cast<T*>(e) = static_cast<T>(raw);
                };
                info.enumOps = ops;
                return info;
            }

            // Names an integer type by signedness and width ("int32", "uint64").
            // If a scalar of the same size and alignment already has that name
            // (long vs long long), T becomes an alias of its TypeInfo.
            template<typename T>
            void registerInteger() {
                static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool> &&
                              !std::is_same_v<T, char> && !isWideCharacter<T>,
                              "registerInteger is for signed/unsigned integer types");
                if (find<T>()) {
                    return;   // e.g. signed char, already registered as std::int8_t
                }
                std::string name = std::string(std::is_signed_v<T> ? "int" : "uint") +
                                   std::to_string(sizeof(T) * CHAR_BIT);
                const TypeInfo* same = find(name);
                if (same && same->kind == TypeKind::Scalar && same->size == sizeof(T) &&
                    same->align == alignof(T)) {
                    _byType.emplace(std::type_index(typeid(T)), same);
                    return;
                }
                registerScalar<T>(std::move(name));
            }

            template<typename T>
            void registerScalar(std::string name) {
                TypeInfo& info = insert<T>(std::move(name), TypeKind::Scalar);
                // operator<< would print these as characters or 1/0, or is
                // deleted (wchar_t, char8_t, char16_t, char32_t).
                if constexpr (std::is_integral_v<T> && sizeof(T) == 1 && !std::is_same_v<T, bool> &&
                              !std::is_same_v<T, char> && !std::is_same_v<T, char8_t>) {
                    // int8 / uint8 (signed / unsigned char): a number, not a character.
                    info.print = [](std::ostream& os, const void* obj) {
                        os << static_cast<int>(*static_cast<const T*>(obj));
                    };
                } else if constexpr (isWideCharacter<T>) {
                    // The numeric code unit: the stream may not be able to
                    // encode the character, and no narrow operator<< exists.
                    info.print = [](std::ostream& os, const void* obj) {
                        using Code = std::conditional_t<std::is_signed_v<T>, long long, unsigned long long>;
                        os << static_cast<Code>(*static_cast<const T*>(obj));
                    };
                } else if constexpr (std::is_same_v<T, bool>) {
                    info.print = [](std::ostream& os, const void* obj) {
                        os << (*static_cast<const bool*>(obj) ? "true" : "false");
                    };
                }
            }

            template<typename T>
            TypeInfo& insert(std::string name, TypeKind kind) {
                const TypeId id = typeId(name);
                if (const TypeInfo* clash = find(id)) {
                    throw std::logic_error("TypeRegistry: '" + name +
                                           "' collides with already registered '" +
                                           clash->name + "'");
                }

                auto info = std::make_unique<TypeInfo>();
                info->name = std::move(name);
                info->id = id;
                info->size = sizeof(T);
                info->align = alignof(T);
                info->kind = kind;
                if constexpr (std::is_default_constructible_v<T>) {
                    info->construct = [](void* dst) { ::new (dst) T(); };
                }
                info->destroy = [](void* obj) noexcept { static_cast<T*>(obj)->~T(); };
                if constexpr (detail::isDeepCopyable<T>()) {
                    info->copy = [](void* dst, const void* src) {
                        ::new (dst) T(*static_cast<const T*>(src));
                    };
                }
                if constexpr (requires(std::ostream& os, const T& value) { os << value; }) {
                    info->print = [](std::ostream& os, const void* obj) {
                        os << *static_cast<const T*>(obj);
                    };
                }

                // TypeInfo lives on the heap: its address survives map rehashes.
                TypeInfo& ref = *info;
                const std::size_t mark = _storage.size();
                _storage.push_back(std::move(info));
                try {
                    _ordered.push_back(&ref);
                    _byId.emplace(id, &ref);
                    _byType.emplace(std::type_index(typeid(T)), &ref);
                } catch (...) {
                    rollback(mark);
                    throw;
                }
                return ref;
            }

            // Removes every TypeInfo stored from index mark on, and every key
            // (aliases included) referring to one of them.
            void rollback(std::size_t mark) noexcept {
                const auto first = _storage.begin() + static_cast<std::ptrdiff_t>(mark);
                const auto removed = [&](const TypeInfo* info) noexcept {
                    return std::any_of(first, _storage.end(),
                                       [info](const auto& owned) { return owned.get() == info; });
                };
                std::erase_if(_byType, [&](const auto& entry) { return removed(entry.second); });
                std::erase_if(_byId, [&](const auto& entry) { return removed(entry.second); });
                _ordered.resize(std::min(_ordered.size(), mark));
                _storage.erase(first, _storage.end());
            }

            // Owns every TypeInfo; _ordered lists them in the same order.
            std::vector<std::unique_ptr<TypeInfo>> _storage;
            // Several keys may share one TypeInfo (integer aliases).
            std::unordered_map<std::type_index, const TypeInfo*> _byType;
            std::unordered_map<TypeId, const TypeInfo*> _byId;
            std::vector<const TypeInfo*> _ordered;
    };
}

// Included last, once TypeRegistry is complete: the catalog defines the
// registration that every E_REFLECT calls, so any reflection header except
// Descriptor.hpp alone is enough to use E_REFLECT.
#include "detail/ComponentCatalog.hpp"
