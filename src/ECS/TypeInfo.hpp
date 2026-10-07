#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ECS::reflect {
    /**
     * @brief Stable identifier of a type, derived from its reflected name.
     *
     * Unlike componentId<T>(), a TypeId does not depend on registration order:
     * the same name always yields the same id, across runs and builds. It is
     * therefore safe to persist in scene files or send over the network.
     */
    using TypeId = std::uint64_t;

    /** @brief 64-bit FNV-1a hash of a type name. */
    [[nodiscard]] constexpr TypeId typeId(std::string_view name) noexcept {
        TypeId hash = 0xcbf29ce484222325ull;
        for (const char c : name) {
            hash ^= static_cast<unsigned char>(c);
            hash *= 0x100000001b3ull;
        }
        return hash;
    }

    /** @brief How a type can be inspected at runtime. */
    enum class TypeKind : std::uint8_t {
        Scalar, ///< Built-in leaf value (bool, integers, floats, std::string).
        Struct, ///< Went through E_REFLECT; described by TypeInfo::fields.
        Opaque, ///< Known size and lifetime ops only; contents not inspectable.
    };

    struct TypeInfo;

    /** @brief Runtime description of one reflected field. */
    struct FieldInfo {
        std::string_view name;
        const TypeInfo* type = nullptr;
        /** @brief Returns the address of this field inside the object at obj. */
        void* (*access)(void* obj) noexcept = nullptr;
    };

    /**
     * @brief Runtime description of a type, usable without knowing it at
     *        compile time (editor, scripts, serialization).
     *
     * Obtained from TypeRegistry; never built by hand. Lifetime operations
     * work on raw storage of `size` bytes aligned to `align`.
     */
    struct TypeInfo {
        std::string name;
        TypeId id = 0;
        std::size_t size = 0;
        std::size_t align = 0;
        TypeKind kind = TypeKind::Opaque;
        std::vector<FieldInfo> fields;

        /** @brief Default-constructs into uninitialized storage; null if T has no default constructor. */
        void (*construct)(void* dst) = nullptr;
        /** @brief Destroys the object at obj; always set. */
        void (*destroy)(void* obj) noexcept = nullptr;
        /** @brief Copy-constructs src into uninitialized storage; null if T is not copyable. */
        void (*copy)(void* dst, const void* src) = nullptr;

        /** @brief Returns the field with this name, or nullptr. */
        [[nodiscard]] const FieldInfo* findField(std::string_view fieldName) const noexcept {
            for (const FieldInfo& field : fields) {
                if (field.name == fieldName) {
                    return &field;
                }
            }
            return nullptr;
        }
    };
}
