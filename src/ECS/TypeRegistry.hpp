#pragma once

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
#include "Reflection.hpp"
#include "TypeInfo.hpp"

namespace ECS::reflect {
    /**
     * @brief Process-wide catalogue of runtime type descriptions.
     *
     * Built-in scalars are registered on construction. Any other type is
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
                    TypeInfo& info = insert<U>(std::string(Descriptor<U>::name), TypeKind::Struct);
                    FieldBuilder<U> builder{*this, info.fields};
                    forEachField<U>(builder);
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
                return it == _byType.end() ? nullptr : it->second.get();
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
                registerScalar<std::int8_t>("int8");
                registerScalar<std::int16_t>("int16");
                registerScalar<std::int32_t>("int32");
                registerScalar<std::int64_t>("int64");
                registerScalar<std::uint8_t>("uint8");
                registerScalar<std::uint16_t>("uint16");
                registerScalar<std::uint32_t>("uint32");
                registerScalar<std::uint64_t>("uint64");
                registerScalar<float>("float");
                registerScalar<double>("double");
                registerScalar<std::string>("string");
            }

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
                        name,
                        &fieldType,
                        [](void* obj) noexcept -> void* {
                            return &(static_cast<T*>(obj)->*Member);
                        },
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

            template<typename T>
            void registerScalar(std::string name) {
                TypeInfo& info = insert<T>(std::move(name), TypeKind::Scalar);
                // operator<< would print these as characters or 1/0.
                if constexpr (std::is_same_v<T, std::int8_t> || std::is_same_v<T, std::uint8_t>) {
                    info.print = [](std::ostream& os, const void* obj) {
                        os << static_cast<int>(*static_cast<const T*>(obj));
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
                _byId.emplace(id, &ref);
                _ordered.push_back(&ref);
                _byType.emplace(std::type_index(typeid(T)), std::move(info));
                return ref;
            }

            std::unordered_map<std::type_index, std::unique_ptr<TypeInfo>> _byType;
            std::unordered_map<TypeId, const TypeInfo*> _byId;
            std::vector<const TypeInfo*> _ordered;
    };
}
