#pragma once

#include <string_view>
#include <type_traits>

namespace ECS::reflect {
    /**
     * @brief Compile-time description of a reflected type.
     * @tparam T Type described.
     *
     * The primary template is intentionally left undefined. It is specialised
     * by E_REFLECT(T), which provides:
     *  - `name`: the type name as written in E_REFLECT.
     *  - `forEach(visitor)`: calls `visitor.field<FieldType>(name, &T::member)`
     *    once per E_FIELD, in declaration order (or `visitor.member<&T::member>(name)`
     *    when the visitor provides it, see detail::visitField).
     */
    template<typename T>
    struct Descriptor;

    /** @brief Satisfied by every type that went through E_REFLECT. */
    template<typename T>
    concept Reflected = requires { Descriptor<T>::name; };

    /** @brief Returns the name given to E_REFLECT for T. */
    template<Reflected T>
    [[nodiscard]] constexpr std::string_view typeName() noexcept {
        return Descriptor<T>::name;
    }

    /**
     * @brief Calls `visitor.field<F>(name, F T::*)` for each reflected field.
     * @tparam T Reflected type.
     * @tparam Visitor Any type exposing a templated `field` member function.
     */
    template<Reflected T, typename Visitor>
    void forEachField(Visitor&& visitor) {
        Descriptor<T>::forEach(visitor);
    }

    namespace detail {
        template<typename MemberPointer>
        struct MemberPointerTraits;

        template<typename C, typename F>
        struct MemberPointerTraits<F C::*> {
            using Class = C;
            using Field = F;
        };

        /**
         * @brief Dispatches one E_FIELD to a visitor.
         * @tparam Member Pointer to the reflected member, as a constant.
         *
         * A visitor exposing `member<Member>(name)` receives the member
         * pointer as a compile-time constant (used to generate captureless
         * accessors for the runtime layer). Otherwise it falls back to
         * `field<F>(name, Member)`, the regular visitor protocol.
         */
        template<auto Member, typename Visitor>
        void visitField(Visitor& visitor, std::string_view name) {
            if constexpr (requires { visitor.template member<Member>(name); }) {
                visitor.template member<Member>(name);
            } else {
                using F = typename MemberPointerTraits<decltype(Member)>::Field;
                visitor.template field<F>(name, Member);
            }
        }

        // Adapts a value-visitor to the member-pointer protocol of Descriptor.
        template<typename Obj, typename Visitor>
        struct BoundVisitor {
            Obj& obj;
            Visitor& visitor;

            template<typename F, typename C>
            void field(std::string_view name, F C::* member) {
                visitor.field(name, obj.*member);
            }
        };
    }

    /**
     * @brief Calls `visitor.field(name, value)` for each reflected field of obj.
     * @param obj Instance to visit; T is deduced (const-qualified allowed).
     * @param visitor Exposes `field(std::string_view, auto&)`; `value` is a
     *        reference to the member (const if obj is const).
     */
    template<typename Obj, typename Visitor>
        requires Reflected<std::remove_const_t<Obj>>
    void forEachField(Obj& obj, Visitor&& visitor) {
        detail::BoundVisitor<Obj, std::remove_reference_t<Visitor>> bound{obj, visitor};
        Descriptor<std::remove_const_t<Obj>>::forEach(bound);
    }
}

/**
 * @brief Opens the reflection block of a type. Must be closed by E_END.
 *
 * Usage:
 * @code
 * E_REFLECT(Position)
 *     E_FIELD(x, float)
 *     E_FIELD(y, float)
 * E_END
 * @endcode
 *
 * Must be used at global scope, after the type definition. Fields must be
 * accessible from outside the type (public, or the type befriends
 * ECS::reflect::Descriptor<T>).
 */
#define E_REFLECT(T)                                                          \
    template<> struct ECS::reflect::Descriptor<T> {                           \
        using Self = T;                                                       \
        static constexpr std::string_view name = #T;                          \
        template<typename Visitor_> static void forEach(Visitor_& v_) {      \
            (void)v_; /* a type with no E_FIELD never uses the visitor */

/**
 * @brief Declares one reflected field. The type may contain commas.
 * @param fname Member name.
 * @param ... Member type; checked against the real declared type.
 */
#define E_FIELD(fname, ...)                                                   \
            static_assert(std::is_same_v<decltype(Self::fname), __VA_ARGS__>, \
                          "E_FIELD: declared type does not match " #fname);   \
            ::ECS::reflect::detail::visitField<&Self::fname>(v_, #fname);

/** @brief Closes an E_REFLECT block. */
#define E_END                                                                 \
        }                                                                     \
    };
