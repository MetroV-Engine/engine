#include "Engine/Serialization/detail/ValueCodec.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace serial::detail {
    using namespace ECS;   // this file only: the ECS types (Entity, reflect...) without prefix

    namespace {
        using reflect::Ref;
        using reflect::TypeInfo;
        using reflect::TypeKind;

        void warn(Report& report, const std::string& path, const std::string& message) {
            report.warnings.push_back(path + ": " + message);
        }

        std::string indexPath(const std::string& path, std::size_t index) {
            return path + "[" + std::to_string(index) + "]";
        }

        std::string fieldPath(const std::string& path, const std::string& field) {
            return path + "." + field;
        }

        // Calls fn(T{}) for each fixed-width integer type until it returns true.
        template<typename Fn>
        bool visitIntegers(Fn&& fn) {
            return fn(std::int8_t{}) || fn(std::int16_t{}) || fn(std::int32_t{}) || fn(std::int64_t{}) ||
                   fn(std::uint8_t{}) || fn(std::uint16_t{}) || fn(std::uint32_t{}) || fn(std::uint64_t{});
        }

        template<typename T>
        bool assignInteger(const Json& json, T& out) {
            if (json.is_number_unsigned()) {
                const auto value = json.get<std::uint64_t>();
                if (value > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) {
                    return false;
                }
                out = static_cast<T>(value);
                return true;
            }
            if (json.is_number_integer()) {
                if constexpr (std::is_unsigned_v<T>) {
                    return false;   // negative
                } else {
                    const auto value = json.get<std::int64_t>();
                    if (value < std::numeric_limits<T>::min() || value > std::numeric_limits<T>::max()) {
                        return false;
                    }
                    out = static_cast<T>(value);
                    return true;
                }
            }
            return false;
        }

        template<typename N>
        bool parseNumber(const std::string& text, N& out) {
            const char* last = text.data() + text.size();
            const auto [ptr, error] = std::from_chars(text.data(), last, out);
            return error == std::errc{} && ptr == last;
        }

        // Shortest text that reads back as the same float, then as a double:
        // 0.1f is written 0.1, not 0.10000000149011612. to_chars and
        // from_chars ignore the C locale (strtod would read "0.1" as 0 where
        // the decimal separator is a comma).
        double shortestDouble(float value) {
            char text[64];
            const auto written = std::to_chars(text, text + sizeof(text), value);
            double result = 0.0;
            std::from_chars(text, written.ptr, result);
            return result;
        }

        // A default-constructed value of a runtime type (a map key read from text).
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
                // A copy of source; empty when its type cannot be copied.
                explicit TempValue(Ref source) : _type(source.type()) {
                    if (!_type.copy) {
                        return;
                    }
                    void* storage = ::operator new(_type.size, std::align_val_t{_type.align});
                    try {
                        _type.copy(storage, source.data());
                    } catch (...) {
                        ::operator delete(storage, std::align_val_t{_type.align});
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
                [[nodiscard]] Ref ref() const noexcept { return _data ? Ref(_type, _data) : Ref{}; }

            private:
                const TypeInfo& _type;
                void* _data = nullptr;
        };

        // --- Map keys: JSON object keys are text ---------------------------------

        std::optional<std::string> keyToText(Ref key) {
            if (const std::string* text = key.tryAs<std::string>()) {
                return *text;
            }
            if (key.is(TypeKind::Enum)) {
                return std::to_string(*key.enumValue());
            }
            std::optional<std::string> result;
            visitIntegers([&](auto tag) {
                using T = decltype(tag);
                const T* value = key.tryAs<T>();
                if (!value) {
                    return false;
                }
                using Wide = std::conditional_t<std::is_signed_v<T>, long long, unsigned long long>;
                result = std::to_string(static_cast<Wide>(*value));
                return true;
            });
            return result;
        }

        bool textToKey(Ref key, const std::string& text) {
            if (std::string* value = key.tryAs<std::string>()) {
                *value = text;
                return true;
            }
            if (key.is(TypeKind::Enum)) {
                std::int64_t raw = 0;
                return parseNumber(text, raw) && key.setEnumValue(raw);
            }
            bool parsed = false;
            visitIntegers([&](auto tag) {
                using T = decltype(tag);
                T* value = key.tryAs<T>();
                if (!value) {
                    return false;
                }
                parsed = parseNumber(text, *value);
                return true;
            });
            return parsed;
        }

        // --- Encoding ------------------------------------------------------------

        Json encodeFloating(double value, EncodeContext& context, const std::string& path) {
            if (!std::isfinite(value)) {
                warn(context.report, path, "a non-finite number cannot be written, saved as null");
                return Json(nullptr);
            }
            return Json(value);
        }

        std::optional<Json> encodeScalar(Ref value, EncodeContext& context, const std::string& path) {
            if (const bool* flag = value.tryAs<bool>()) {
                return Json(*flag);
            }
            if (const std::string* text = value.tryAs<std::string>()) {
                return Json(*text);
            }
            if (const char* character = value.tryAs<char>()) {
                // char is signed on some platforms and unsigned on others:
                // always write 0..255 so the text means the same everywhere.
                return Json(static_cast<int>(static_cast<unsigned char>(*character)));
            }
            if (const float* number = value.tryAs<float>()) {
                return std::isfinite(*number) ? Json(shortestDouble(*number))
                                              : encodeFloating(static_cast<double>(*number), context, path);
            }
            if (const double* number = value.tryAs<double>()) {
                return encodeFloating(*number, context, path);
            }
            std::optional<Json> result;
            visitIntegers([&](auto tag) {
                using T = decltype(tag);
                const T* number = value.tryAs<T>();
                if (!number) {
                    return false;
                }
                result = Json(*number);
                return true;
            });
            if (!result) {
                warn(context.report, path, "scalar type '" + value.type().name + "' is not supported, skipped");
            }
            return result;
        }

        Json encodeEntity(const Entity& entity, EncodeContext& context, const std::string& path) {
            if (entity.world() == InvalidWorld) {
                return Json(nullptr);   // a handle that refers to nothing
            }
            if (!context.entities) {
                warn(context.report, path, "an entity reference cannot be written without a world, saved as null");
                return Json(nullptr);
            }
            const auto it = context.entities->find(entity);
            if (it == context.entities->end()) {
                warn(context.report, path, "the referenced entity is not in the world, saved as null");
                return Json(nullptr);
            }
            return Json(uuid::toString(it->second));
        }

        std::optional<Json> encodeMap(Ref value, EncodeContext& context, const std::string& path) {
            std::vector<std::pair<std::string, Json>> entries;
            bool supported = true;
            value.forEachEntry([&](Ref key, Ref entry) {
                const std::optional<std::string> text = keyToText(key);
                if (!text) {
                    supported = false;
                    return;
                }
                if (auto json = encode(entry, context, path + "[" + *text + "]")) {
                    entries.emplace_back(*text, std::move(*json));
                }
            });
            if (!supported) {
                warn(context.report, path, "keys of type '" + value.type().mapOps->key->name + "' are not supported, skipped");
                return std::nullopt;
            }
            // Unordered maps iterate in any order: sort so that the text does not change for nothing.
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            Json object = Json::object();
            for (auto& [text, json] : entries) {
                object[text] = std::move(json);
            }
            return object;
        }

        // --- Decoding ------------------------------------------------------------

        void decodeScalar(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
            if (bool* flag = target.tryAs<bool>()) {
                if (json.is_boolean()) {
                    *flag = json.get<bool>();
                } else {
                    warn(context.report, path, "expected a boolean, kept the current value");
                }
                return;
            }
            if (std::string* text = target.tryAs<std::string>()) {
                if (json.is_string()) {
                    *text = json.get<std::string>();
                } else {
                    warn(context.report, path, "expected a string, kept the current value");
                }
                return;
            }
            if (float* number = target.tryAs<float>()) {
                if (!json.is_number()) {
                    warn(context.report, path, "expected a number, kept the current value");
                } else if (const float converted = static_cast<float>(json.get<double>()); std::isfinite(converted)) {
                    *number = converted;
                } else {
                    warn(context.report, path, "the number is too large for a float, kept the current value");
                }
                return;
            }
            if (double* number = target.tryAs<double>()) {
                if (json.is_number()) {
                    *number = json.get<double>();
                } else {
                    warn(context.report, path, "expected a number, kept the current value");
                }
                return;
            }
            if (char* character = target.tryAs<char>()) {
                int raw = 0;
                if (assignInteger(json, raw) && raw >= 0 && raw <= 255) {
                    *character = static_cast<char>(static_cast<unsigned char>(raw));
                } else {
                    warn(context.report, path, "expected a character code from 0 to 255, kept the current value");
                }
                return;
            }
            const bool handled = visitIntegers([&](auto tag) {
                using T = decltype(tag);
                T* number = target.tryAs<T>();
                if (!number) {
                    return false;
                }
                if (!assignInteger(json, *number)) {
                    warn(context.report, path, "expected an integer in range for '" + target.type().name +
                                                    "', kept the current value");
                }
                return true;
            });
            if (!handled) {
                warn(context.report, path, "scalar type '" + target.type().name + "' is not supported, skipped");
            }
        }

        void decodeEntity(const Json& json, Entity& target, DecodeContext& context, const std::string& path) {
            if (json.is_null()) {
                target = Entity{};
                return;
            }
            if (!json.is_string()) {
                warn(context.report, path, "expected an entity id (a string) or null, kept the current value");
                return;
            }
            if (!context.entities) {
                warn(context.report, path, "an entity reference cannot be resolved without a world, skipped");
                return;
            }
            const std::optional<Uuid> id = uuid::fromString(json.get<std::string>());
            const auto it = id ? context.entities->find(*id) : context.entities->end();
            if (it == context.entities->end()) {
                warn(context.report, path, "the referenced entity '" + json.get<std::string>() +
                                                "' is not in the text, set to no entity");
                target = Entity{};
                return;
            }
            target = it->second;
        }

        void decodeStruct(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
            if (!json.is_object()) {
                warn(context.report, path, "expected an object, kept the current value");
                return;
            }
            for (auto it = json.begin(); it != json.end(); ++it) {
                if (!target.type().findField(it.key())) {
                    warn(context.report, fieldPath(path, it.key()), "unknown field, ignored");
                }
            }
            for (const reflect::FieldInfo& field : target.type().fields) {
                if (!json.contains(field.name)) {
                    continue;   // a missing field keeps its default value
                }
                if (field.readOnly) {
                    warn(context.report, fieldPath(path, field.name), "read-only field, ignored");
                    continue;
                }
                decode(json[field.name], target.field(field), context, fieldPath(path, field.name));
            }
        }

        void decodeSequence(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
            if (!json.is_array()) {
                warn(context.report, path, "expected an array, kept the current value");
                return;
            }
            std::size_t count = json.size();
            if (target.resizable()) {
                target.resize(count);
            } else if (target.size() != count) {
                warn(context.report, path, "expected " + std::to_string(target.size()) + " elements, got " +
                                                std::to_string(count));
                count = std::min(count, target.size());
            }
            for (std::size_t i = 0; i < count; ++i) {
                decode(json[i], target.at(i), context, indexPath(path, i));
            }
        }

        // Removes every entry: like a sequence, a map takes the content of the
        // text, it is not merged with what it held. The keys are copied first,
        // as a map must not change while it is walked.
        void clearMap(Ref map, DecodeContext& context, const std::string& path) {
            std::vector<std::unique_ptr<TempValue>> keys;
            bool copyable = true;
            map.forEachEntry([&](Ref key, Ref) {
                keys.push_back(std::make_unique<TempValue>(key));
                copyable = copyable && keys.back()->ref().valid();
            });
            if (!copyable) {
                warn(context.report, path, "the keys cannot be copied, the existing entries are kept");
                return;
            }
            for (const auto& key : keys) {
                map.eraseKey(key->ref());
            }
        }

        void decodeMap(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
            if (!json.is_object()) {
                warn(context.report, path, "expected an object, kept the current value");
                return;
            }
            clearMap(target, context, path);
            const TempValue key(*target.type().mapOps->key);
            for (auto it = json.begin(); it != json.end(); ++it) {
                const std::string entryPath = path + "[" + it.key() + "]";
                if (!key.ref() || !textToKey(key.ref(), it.key())) {
                    warn(context.report, entryPath, "the key cannot be read as a '" +
                                                         target.type().mapOps->key->name + "', skipped");
                    continue;
                }
                const Ref entry = target.insert(key.ref());
                if (!entry) {
                    warn(context.report, entryPath, "the value type has no default constructor, skipped");
                    continue;
                }
                decode(it.value(), entry, context, entryPath);
            }
        }

        void decodeOptional(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
            if (json.is_null()) {
                target.reset();
                return;
            }
            const Ref value = target.emplace();
            if (!value) {
                warn(context.report, path, "the value type has no default constructor, skipped");
                return;
            }
            decode(json, value, context, path);
        }
    }

    std::optional<Json> encode(Ref value, EncodeContext& context, const std::string& path) {
        if (!value.valid()) {
            return std::nullopt;
        }
        if (const Uuid* id = value.tryAs<Uuid>()) {
            return Json(uuid::toString(*id));
        }
        if (const Entity* entity = value.tryAs<Entity>()) {
            return encodeEntity(*entity, context, path);
        }
        switch (value.type().kind) {
            case TypeKind::Scalar:
                return encodeScalar(value, context, path);
            case TypeKind::Enum:
                return Json(*value.enumValue());
            case TypeKind::Struct: {
                Json object = Json::object();
                for (const reflect::FieldInfo& field : value.type().fields) {
                    if (field.readOnly) {
                        continue;   // a const member cannot be restored: writing it would be noise
                    }
                    if (auto json = encode(value.field(field), context, fieldPath(path, field.name))) {
                        object[field.name] = std::move(*json);
                    }
                }
                return object;
            }
            case TypeKind::Sequence: {
                Json array = Json::array();
                for (std::size_t i = 0; i < value.size(); ++i) {
                    auto json = encode(value.at(i), context, indexPath(path, i));
                    array.push_back(json ? std::move(*json) : Json(nullptr));   // keep the indices aligned
                }
                return array;
            }
            case TypeKind::Map:
                return encodeMap(value, context, path);
            case TypeKind::Optional: {
                if (!value.hasValue()) {
                    return Json(nullptr);
                }
                return encode(value.value(), context, path);
            }
            case TypeKind::Opaque:
                break;
        }
        warn(context.report, path, "type '" + value.type().name + "' is not serializable, skipped");
        return std::nullopt;
    }

    void decode(const Json& json, Ref target, DecodeContext& context, const std::string& path) {
        if (!target.valid()) {
            return;
        }
        if (Uuid* id = target.tryAs<Uuid>()) {
            const std::optional<Uuid> parsed =
                json.is_string() ? uuid::fromString(json.get<std::string>()) : std::nullopt;
            if (parsed) {
                *id = *parsed;
            } else {
                warn(context.report, path, "expected an id like xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, kept the current value");
            }
            return;
        }
        if (Entity* entity = target.tryAs<Entity>()) {
            decodeEntity(json, *entity, context, path);
            return;
        }
        switch (target.type().kind) {
            case TypeKind::Scalar:
                decodeScalar(json, target, context, path);
                return;
            case TypeKind::Enum:
                if (json.is_number_integer() && !(json.is_number_unsigned() &&
                                                  json.get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))) {
                    // setEnumValue truncates to the enum's underlying type: read it back to detect a value that does not fit.
                    const std::int64_t raw = json.get<std::int64_t>();
                    const std::int64_t previous = *target.enumValue();
                    target.setEnumValue(raw);
                    if (*target.enumValue() != raw) {
                        target.setEnumValue(previous);
                        warn(context.report, path, "the value " + std::to_string(raw) +
                                                       " does not fit the enum's type, kept the current value");
                    }
                } else {
                    warn(context.report, path, "expected an integer, kept the current value");
                }
                return;
            case TypeKind::Struct:
                decodeStruct(json, target, context, path);
                return;
            case TypeKind::Sequence:
                decodeSequence(json, target, context, path);
                return;
            case TypeKind::Map:
                decodeMap(json, target, context, path);
                return;
            case TypeKind::Optional:
                decodeOptional(json, target, context, path);
                return;
            case TypeKind::Opaque:
                break;
        }
        warn(context.report, path, "type '" + target.type().name + "' is not serializable, skipped");
    }
}
