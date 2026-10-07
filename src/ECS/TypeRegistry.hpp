#pragma once

#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

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
     *  - anything else becomes TypeKind::Opaque (named after typeid(T).name(),
     *    which is compiler-specific: do not persist the id of an Opaque type).
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

                if (const TypeInfo* existing = find<U>()) {
                    return *existing;
                }
                if constexpr (Reflected<U>) {
                    TypeInfo& info = insert<U>(std::string(Descriptor<U>::name), TypeKind::Struct);
                    FieldBuilder<U> builder{*this, info.fields};
                    forEachField<U>(builder);
                    return info;
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

            template<typename T>
            void registerScalar(std::string name) {
                insert<T>(std::move(name), TypeKind::Scalar);
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
                if constexpr (std::is_copy_constructible_v<T>) {
                    info->copy = [](void* dst, const void* src) {
                        ::new (dst) T(*static_cast<const T*>(src));
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
