#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "TypeInfo.hpp"

namespace ECS::reflect {
    /**
     * @brief Customisation point teaching the runtime layer about a container.
     * @tparam T Container type.
     *
     * Left undefined: a type without specialisation is not a container. A
     * specialisation declares `static constexpr TypeKind kind` and, per kind:
     *
     * - Sequence: `using Element`, `name(elementName)`, `size(const T&)`,
     *   `at(T&, index) -> Element*`; optionally `resize(T&, count)` and
     *   `erase(T&, index)` (absent means fixed size).
     * - Map: `using Key`, `using Value`, `name(keyName, valueName)`,
     *   `size(const T&)`, `find(T&, const Key&) -> Value*` (null if absent),
     *   `erase(T&, const Key&) -> bool`, `forEach(T&, fn(const Key&, Value&))`;
     *   optionally `insert(T&, const Key&) -> Value*`.
     * - Optional: `using Value`, `name(valueName)`, `hasValue(const T&)`,
     *   `get(T&) -> Value*`, `reset(T&)`; optionally `emplace(T&) -> Value*`.
     *
     * Specialise it next to your own container type to make it inspectable
     * by the editor, scripts and serializers without touching the engine.
     * Optional members are detected by the TypeRegistry: constrain them with
     * `requires` when they only exist for some element types.
     */
    template<typename T>
    struct ContainerTraits;

    /** @brief Satisfied by types that have a ContainerTraits specialisation. */
    template<typename T>
    concept HasContainerTraits = requires { ContainerTraits<T>::kind; };

    // --- Sequences --------------------------------------------------------

    template<typename E>
    struct ContainerTraits<std::vector<E>> {
        using Container = std::vector<E>;
        static constexpr TypeKind kind = TypeKind::Sequence;
        using Element = E;

        static std::string name(std::string_view element) {
            return "vector<" + std::string(element) + ">";
        }
        static std::size_t size(const Container& c) noexcept { return c.size(); }
        static E* at(Container& c, std::size_t index) noexcept { return &c[index]; }
        static void resize(Container& c, std::size_t count)
            requires std::is_default_constructible_v<E> {
            c.resize(count);
        }
        static void erase(Container& c, std::size_t index) {
            c.erase(c.begin() + static_cast<std::ptrdiff_t>(index));
        }
    };

    // std::vector<bool> packs its elements as bits: they have no address, so
    // it cannot be exposed element by element. It stays Opaque.
    template<>
    struct ContainerTraits<std::vector<bool>> {};

    template<typename E, std::size_t N>
    struct ContainerTraits<std::array<E, N>> {
        using Container = std::array<E, N>;
        static constexpr TypeKind kind = TypeKind::Sequence;
        using Element = E;

        static std::string name(std::string_view element) {
            return "array<" + std::string(element) + "," + std::to_string(N) + ">";
        }
        static std::size_t size(const Container&) noexcept { return N; }
        static E* at(Container& c, std::size_t index) noexcept { return &c[index]; }
    };

    // --- Maps -------------------------------------------------------------

    namespace detail {
        template<typename M, typename K, typename V>
        struct AssociativeTraits {
            static constexpr TypeKind kind = TypeKind::Map;
            using Key = K;
            using Value = V;

            static std::size_t size(const M& m) noexcept { return m.size(); }
            static V* find(M& m, const K& key) {
                const auto it = m.find(key);
                return it == m.end() ? nullptr : &it->second;
            }
            static V* insert(M& m, const K& key) requires std::is_default_constructible_v<V> {
                return &m.try_emplace(key).first->second;
            }
            static bool erase(M& m, const K& key) { return m.erase(key) > 0; }
            template<typename Fn>
            static void forEach(M& m, Fn&& fn) {
                for (auto& [key, value] : m) {
                    fn(key, value);
                }
            }
        };
    }

    template<typename K, typename V>
    struct ContainerTraits<std::map<K, V>> : detail::AssociativeTraits<std::map<K, V>, K, V> {
        static std::string name(std::string_view key, std::string_view value) {
            return "map<" + std::string(key) + "," + std::string(value) + ">";
        }
    };

    template<typename K, typename V>
    struct ContainerTraits<std::unordered_map<K, V>>
        : detail::AssociativeTraits<std::unordered_map<K, V>, K, V> {
        static std::string name(std::string_view key, std::string_view value) {
            return "unordered_map<" + std::string(key) + "," + std::string(value) + ">";
        }
    };

    // --- Optional ---------------------------------------------------------

    template<typename T>
    struct ContainerTraits<std::optional<T>> {
        using Container = std::optional<T>;
        static constexpr TypeKind kind = TypeKind::Optional;
        using Value = T;

        static std::string name(std::string_view value) {
            return "optional<" + std::string(value) + ">";
        }
        static bool hasValue(const Container& o) noexcept { return o.has_value(); }
        static T* get(Container& o) noexcept { return &*o; }
        static T* emplace(Container& o) requires std::is_default_constructible_v<T> {
            return &o.emplace();
        }
        static void reset(Container& o) noexcept { o.reset(); }
    };

    namespace detail {
        /**
         * @brief is_copy_constructible that also looks inside containers.
         *
         * std::vector<std::unique_ptr<X>> claims to be copy-constructible but
         * fails to compile when copied; this checks the elements instead.
         * A user struct holding such a container still claims to be copyable:
         * declare its copy constructor `= delete` to make it explicit.
         */
        template<typename T>
        constexpr bool isDeepCopyable() {
            if constexpr (!std::is_copy_constructible_v<T>) {
                return false;
            } else if constexpr (HasContainerTraits<T>) {
                using Traits = ContainerTraits<T>;
                if constexpr (Traits::kind == TypeKind::Map) {
                    return isDeepCopyable<typename Traits::Key>() &&
                           isDeepCopyable<typename Traits::Value>();
                } else if constexpr (Traits::kind == TypeKind::Optional) {
                    return isDeepCopyable<typename Traits::Value>();
                } else {
                    return isDeepCopyable<typename Traits::Element>();
                }
            } else {
                return true;
            }
        }
    }
}
