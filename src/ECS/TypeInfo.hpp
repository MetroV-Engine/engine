#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
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
        Scalar,   ///< Built-in leaf value (bool, integers, floats, std::string).
        Struct,   ///< Went through E_REFLECT; described by TypeInfo::fields.
        Sequence, ///< Indexed container (std::vector, std::array); see SequenceOps.
        Map,      ///< Key/value container (std::map, std::unordered_map); see MapOps.
        Optional, ///< std::optional; see OptionalOps.
        Enum,     ///< Enumeration, handled through its integer value; see EnumOps.
        Opaque,   ///< Known size and lifetime ops only; contents not inspectable.
    };

    struct TypeInfo;

    /** @brief Runtime operations of a TypeKind::Sequence. */
    struct SequenceOps {
        const TypeInfo* element = nullptr;
        std::size_t (*size)(const void* container) noexcept = nullptr;
        /** @brief Address of an element; index must be < size (unchecked). */
        void* (*at)(void* container, std::size_t index) noexcept = nullptr;
        /** @brief Null for fixed-size sequences or non default-constructible elements. */
        void (*resize)(void* container, std::size_t count) = nullptr;
        /** @brief Null for fixed-size sequences; index must be < size (unchecked). */
        void (*erase)(void* container, std::size_t index) = nullptr;
    };

    /** @brief Runtime operations of a TypeKind::Map. Keys are passed as pointers to a Key. */
    struct MapOps {
        const TypeInfo* key = nullptr;
        const TypeInfo* value = nullptr;
        std::size_t (*size)(const void* map) noexcept = nullptr;
        /** @brief Address of the value for key, or null when absent. */
        void* (*find)(void* map, const void* key) = nullptr;
        /** @brief Inserts a default value if absent; returns its address. Null if Value has no default constructor. */
        void* (*insert)(void* map, const void* key) = nullptr;
        /** @brief Removes key; returns whether it was present. */
        bool (*erase)(void* map, const void* key) = nullptr;
        /** @brief Calls fn(ctx, key, value) for each entry; the map must not be modified meanwhile. */
        void (*forEach)(void* map, void* ctx,
                        void (*fn)(void* ctx, const void* key, void* value)) = nullptr;
    };

    /** @brief Runtime operations of a TypeKind::Optional. */
    struct OptionalOps {
        const TypeInfo* value = nullptr;
        bool (*hasValue)(const void* optional) noexcept = nullptr;
        /** @brief Address of the contained value; requires hasValue (unchecked). */
        void* (*get)(void* optional) noexcept = nullptr;
        /** @brief Default-constructs the value (replacing any); null if Value has no default constructor. */
        void* (*emplace)(void* optional) = nullptr;
        void (*reset)(void* optional) noexcept = nullptr;
    };

    /** @brief Runtime operations of a TypeKind::Enum, through its integer value. */
    struct EnumOps {
        const TypeInfo* underlying = nullptr;
        std::int64_t (*get)(const void* value) noexcept = nullptr;
        void (*set)(void* value, std::int64_t raw) noexcept = nullptr;
    };

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
        /** @brief Writes the value with `os << value`; null if T has no operator<<. */
        void (*print)(std::ostream& os, const void* obj) = nullptr;

        /** @brief Set only for the matching kind. */
        std::optional<SequenceOps> sequenceOps;
        std::optional<MapOps> mapOps;
        std::optional<OptionalOps> optionalOps;
        std::optional<EnumOps> enumOps;

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
