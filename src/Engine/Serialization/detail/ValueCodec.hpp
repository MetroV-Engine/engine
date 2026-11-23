#pragma once

// Internal: converts a reflected value (Ref) to and from JSON. Not part of
// the public API of the module (it exposes the JSON library).

#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "ECS/Core/Entity/Entity.hpp"
#include "Engine/Components/Uuid.hpp"
#include "ECS/Reflection/Ref.hpp"
#include "Engine/Serialization/Serialization.hpp"

namespace serial::detail {
    // Keeps the declaration order of the fields in the output.
    using Json = nlohmann::ordered_json;

    struct EncodeContext {
        Report& report;
        /** @brief Uuid of every entity of the world; null when there is no world. */
        const std::map<ECS::Entity, Uuid>* entities = nullptr;
    };

    struct DecodeContext {
        Report& report;
        /** @brief Entity created for every Uuid of the text; null when there is no world. */
        const std::unordered_map<Uuid, ECS::Entity>* entities = nullptr;
    };

    /**
     * @brief JSON of a value; nullopt when the value cannot be written (the
     *        reason is a warning in the context's report).
     * @param path Where the value is, for the warnings ("Health.max").
     */
    [[nodiscard]] std::optional<Json> encode(ECS::reflect::Ref value, EncodeContext& context, const std::string& path);

    /** @brief Writes json into target; whatever cannot be read is a warning and keeps its value. */
    void decode(const Json& json, ECS::reflect::Ref target, DecodeContext& context, const std::string& path);
}
