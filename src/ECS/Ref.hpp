#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

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
     * A Ref does not extend the lifetime of the object it points to. Refs to
     * container elements are invalidated by anything that invalidates the
     * container's own references (resize, push, erase, insert on a rehash...).
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

            /** @brief True when the Ref is valid and of this kind. */
            [[nodiscard]] bool is(TypeKind kind) const noexcept {
                return valid() && _type->kind == kind;
            }

            /** @brief Type of the pointed object; requires valid(). */
            [[nodiscard]] const TypeInfo& type() const noexcept { return *_type; }

            /** @brief Raw address of the pointed object (null if invalid). */
            [[nodiscard]] void* data() const noexcept { return _data; }

            // --- Struct ---------------------------------------------------

            /**
             * @brief Returns a Ref to the named field.
             * @return An invalid Ref if this Ref is invalid or has no such field,
             *         so lookups can be chained: `ref.field("pos").field("x")`.
             *         Does not look through an Optional (path() does).
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
             * @brief Follows a path of field names, indices and keys.
             * @param path e.g. "transform.pos.x", "tags[2]", "weights[head]",
             *        "inventory[0].count". A field name or index applied to an
             *        Optional goes through its value ("weapon.damage").
             *        Map keys are parsed from text: string, integer and enum
             *        keys are supported.
             * @return This Ref for an empty path; an invalid Ref if any step
             *         fails (missing field, index out of range, missing key,
             *         empty Optional, malformed path such as "a..b" or "a.").
             */
            [[nodiscard]] Ref path(std::string_view path) const {
                if (path.empty()) {
                    return *this;
                }
                Ref current = *this;
                std::size_t pos = 0;
                while (true) {
                    const std::size_t end = std::min(path.find_first_of(".[", pos), path.size());
                    const std::string_view name = path.substr(pos, end - pos);
                    if (!name.empty()) {
                        current = current.unwrapped().field(name);
                    } else if (pos != 0 || path[end] != '[') {
                        // Empty segment; only "[i]..." may start a path. At pos 0 the path is
                        // non-empty and starts with '.' or '[', so path[end] is in range.
                        return {};
                    }
                    pos = end;
                    while (pos < path.size() && path[pos] == '[') {
                        const std::size_t close = path.find(']', pos);
                        if (close == std::string_view::npos) {
                            return {};
                        }
                        current = current.unwrapped().element(path.substr(pos + 1, close - pos - 1));
                        pos = close + 1;
                    }
                    if (!current || pos == path.size()) {
                        return current;
                    }
                    if (path[pos] != '.') {
                        return {};
                    }
                    ++pos;
                }
            }

            // --- Sequence and Map -----------------------------------------

            /** @brief Element count of a Sequence or entry count of a Map; 0 otherwise. */
            [[nodiscard]] std::size_t size() const noexcept {
                if (is(TypeKind::Sequence)) {
                    return _type->sequenceOps->size(_data);
                }
                if (is(TypeKind::Map)) {
                    return _type->mapOps->size(_data);
                }
                return 0;
            }

            // --- Sequence -------------------------------------------------

            /** @brief Ref to the element at index; invalid if not a Sequence or out of range. */
            [[nodiscard]] Ref at(std::size_t index) const noexcept {
                if (!is(TypeKind::Sequence) || index >= size()) {
                    return {};
                }
                const SequenceOps& ops = *_type->sequenceOps;
                return Ref(*ops.element, ops.at(_data, index));
            }

            /** @brief True for a Sequence that can change size (std::vector, not std::array). */
            [[nodiscard]] bool resizable() const noexcept {
                return is(TypeKind::Sequence) && _type->sequenceOps->resize;
            }

            /** @brief Resizes a resizable Sequence; returns false otherwise. */
            bool resize(std::size_t count) const {
                if (!resizable()) {
                    return false;
                }
                _type->sequenceOps->resize(_data, count);
                return true;
            }

            /** @brief Appends a default element; returns a Ref to it, or an invalid Ref. */
            Ref push() const {
                const std::size_t count = size();
                return resize(count + 1) ? at(count) : Ref{};
            }

            /** @brief Removes the element at index; returns false if impossible. */
            bool erase(std::size_t index) const {
                if (!is(TypeKind::Sequence) || !_type->sequenceOps->erase || index >= size()) {
                    return false;
                }
                _type->sequenceOps->erase(_data, index);
                return true;
            }

            // --- Map ------------------------------------------------------

            /** @brief Ref to the value for key; invalid if absent or key has the wrong type. */
            [[nodiscard]] Ref find(Ref key) const {
                if (!acceptsKey(key)) {
                    return {};
                }
                const MapOps& ops = *_type->mapOps;
                void* value = ops.find(_data, key._data);
                return value ? Ref(*ops.value, value) : Ref{};
            }

            /** @brief Inserts a default value for key if absent; returns a Ref to the value. */
            Ref insert(Ref key) const {
                if (!acceptsKey(key) || !_type->mapOps->insert) {
                    return {};
                }
                const MapOps& ops = *_type->mapOps;
                return Ref(*ops.value, ops.insert(_data, key._data));
            }

            /** @brief Removes key; returns whether it was present. */
            bool erase(Ref key) const {
                return acceptsKey(key) && _type->mapOps->erase(_data, key._data);
            }

            /**
             * @brief Calls fn(Ref key, Ref value) for each entry of a Map.
             *
             * The key Ref must be treated as read-only, and the map must not
             * be modified during the iteration.
             */
            template<typename Fn>
            void forEachEntry(Fn&& fn) const {
                if (!is(TypeKind::Map)) {
                    return;
                }
                struct Context {
                    std::remove_reference_t<Fn>* fn;
                    const MapOps* ops;
                };
                Context context{&fn, &*_type->mapOps};
                _type->mapOps->forEach(_data, &context, [](void* ctx, const void* key, void* value) {
                    auto& c = *static_cast<Context*>(ctx);
                    (*c.fn)(Ref(*c.ops->key, const_cast<void*>(key)), Ref(*c.ops->value, value));
                });
            }

            // --- Optional -------------------------------------------------

            /** @brief True for an Optional holding a value. */
            [[nodiscard]] bool hasValue() const noexcept {
                return is(TypeKind::Optional) && _type->optionalOps->hasValue(_data);
            }

            /** @brief Ref to the contained value; invalid if empty or not an Optional. */
            [[nodiscard]] Ref value() const noexcept {
                if (!hasValue()) {
                    return {};
                }
                const OptionalOps& ops = *_type->optionalOps;
                return Ref(*ops.value, ops.get(_data));
            }

            /** @brief Default-constructs the value (replacing any); returns a Ref to it. */
            Ref emplace() const {
                if (!is(TypeKind::Optional) || !_type->optionalOps->emplace) {
                    return {};
                }
                const OptionalOps& ops = *_type->optionalOps;
                return Ref(*ops.value, ops.emplace(_data));
            }

            /** @brief Empties an Optional; returns false if not an Optional. */
            bool reset() const noexcept {
                if (!is(TypeKind::Optional)) {
                    return false;
                }
                _type->optionalOps->reset(_data);
                return true;
            }

            // --- Enum -----------------------------------------------------

            /** @brief Integer value of an Enum; nullopt if not an Enum. */
            [[nodiscard]] std::optional<std::int64_t> enumValue() const noexcept {
                if (!is(TypeKind::Enum)) {
                    return std::nullopt;
                }
                return _type->enumOps->get(_data);
            }

            /** @brief Sets an Enum from its integer value; returns false if not an Enum. */
            bool setEnumValue(std::int64_t raw) const noexcept {
                if (!is(TypeKind::Enum)) {
                    return false;
                }
                _type->enumOps->set(_data, raw);
                return true;
            }

            // --- Typed access ---------------------------------------------

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
            // Owns a default-constructed temporary of a runtime type (map keys parsed from text).
            class TempValue {
                public:
                    explicit TempValue(const TypeInfo& type) : _type(type) {
                        if (!type.construct) {
                            return;
                        }
                        void* storage = ::operator new(type.size, std::align_val_t{type.align});
                        try {
                            type.construct(storage);
                        } catch (...) {
                            ::operator delete(storage, std::align_val_t{type.align});
                            throw;
                        }
                        _data = storage;
                    }
                    TempValue(const TempValue&) = delete;
                    TempValue& operator=(const TempValue&) = delete;
                    ~TempValue() {
                        if (_data) {
                            _type.destroy(_data);
                            ::operator delete(_data, std::align_val_t{_type.align});
                        }
                    }
                    [[nodiscard]] Ref ref() const noexcept {
                        return _data ? Ref(_type, _data) : Ref{};
                    }

                private:
                    const TypeInfo& _type;
                    void* _data = nullptr;
            };

            [[nodiscard]] bool acceptsKey(Ref key) const noexcept {
                return is(TypeKind::Map) && key.valid() && key._type == _type->mapOps->key;
            }

            // Goes through Optionals so that path() can descend into their value.
            [[nodiscard]] Ref unwrapped() const noexcept {
                Ref current = *this;
                while (current.is(TypeKind::Optional)) {
                    current = current.value();
                }
                return current;
            }

            // One "[...]" step of path(): an index for a Sequence, a key for a Map.
            [[nodiscard]] Ref element(std::string_view text) const {
                if (is(TypeKind::Sequence)) {
                    std::size_t index = 0;
                    return parseNumber(text, index) ? at(index) : Ref{};
                }
                if (is(TypeKind::Map)) {
                    const TempValue key(*_type->mapOps->key);
                    return parseInto(key.ref(), text) ? find(key.ref()) : Ref{};
                }
                return {};
            }

            template<typename N>
            static bool parseNumber(std::string_view text, N& out) noexcept {
                const char* last = text.data() + text.size();
                const auto [ptr, error] = std::from_chars(text.data(), last, out);
                return error == std::errc{} && ptr == last;   // from_chars rejects empty text
            }

            template<typename N>
            static bool parseIntegral(Ref target, std::string_view text, bool& matched) noexcept {
                N* value = target.tryAs<N>();
                if (!value) {
                    return false;
                }
                matched = true;
                return parseNumber(text, *value);
            }

            // Writes text into target; supports string, integer and enum values.
            static bool parseInto(Ref target, std::string_view text) {
                if (!target.valid()) {
                    return false;
                }
                if (std::string* value = target.tryAs<std::string>()) {
                    *value = text;
                    return true;
                }
                if (target.is(TypeKind::Enum)) {
                    std::int64_t raw = 0;
                    if (!parseNumber(text, raw)) {
                        return false;
                    }
                    target.setEnumValue(raw);   // cannot fail: target is an Enum
                    return true;
                }
                bool matched = false;
                const bool parsed =
                    parseIntegral<std::int8_t>(target, text, matched) ||
                    parseIntegral<std::int16_t>(target, text, matched) ||
                    parseIntegral<std::int32_t>(target, text, matched) ||
                    parseIntegral<std::int64_t>(target, text, matched) ||
                    parseIntegral<std::uint8_t>(target, text, matched) ||
                    parseIntegral<std::uint16_t>(target, text, matched) ||
                    parseIntegral<std::uint32_t>(target, text, matched) ||
                    parseIntegral<std::uint64_t>(target, text, matched);
                return matched && parsed;
            }

            const TypeInfo* _type = nullptr;
            void* _data = nullptr;
    };
}
