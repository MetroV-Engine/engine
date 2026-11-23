#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "ECS/Core/Registry.hpp"
#include "ECS/Reflection/Ref.hpp"

/**
 * @file
 * @brief Saves and loads a world, or any reflected value, as text.
 *
 * These functions are the whole public surface. How the text is produced
 * (JSON today) and with which library is private to this module.
 */
namespace serial {
    /** @brief Version of the world format written by serialize(). */
    inline constexpr int FormatVersion = 1;

    /**
     * @brief What went wrong, and what was tolerated, during one call.
     *
     * Every function appends to the Report it is given and never clears it,
     * so one Report can collect several calls. A call succeeds when it adds
     * no error, whatever the Report already held. A warning means something
     * was skipped or defaulted (unknown component, unsupported field, wrong
     * type...) and the call went on.
     */
    struct Report {
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        [[nodiscard]] bool ok() const noexcept { return errors.empty(); }
    };

    /** @brief Options of serialize() and saveFile(). */
    struct SaveOptions {
        /**
         * @brief Gives a Uuid component to every entity that has none (or a
         *        null one), so that its id is the same in every later save.
         *
         * Off by default: the world is not modified, and an entity without a
         * Uuid gets a new temporary id in each text. An editor saving a scene
         * turns it on, so that saving twice writes the same ids and the files
         * stay readable in a diff. An entity whose Uuid is used by another
         * entity is left as it is, with a warning.
         */
        bool assignMissingUuids = false;
    };

    // --- A whole world ------------------------------------------------------

    /**
     * @brief Writes every entity of the world and its components.
     *
     * Entities are identified by their Uuid. One that has none gets a
     * temporary Uuid in the text only: the world is not modified (unless
     * options.assignMissingUuids is set). References
     * to entities (fields of type ECS::Entity) are written as the Uuid of
     * their target, or null when the target is not in this world.
     * Fields that cannot be written (Opaque types, unsupported scalars) are
     * skipped with a warning.
     * @return The text, or an empty string with an error in the report.
     */
    [[nodiscard]] std::string serialize(ECS::Registry& world, Report* report = nullptr, const SaveOptions& options = {});

    /**
     * @brief Creates the entities described by the text, in addition to the
     *        ones already in the world.
     *
     * Reading is tolerant: an unknown component, an unknown field or a
     * value of the wrong type is skipped with a warning and the default
     * value stays. A malformed text, an unsupported format version, or a
     * Uuid that is duplicated or already in the world are errors: nothing is
     * created. References between entities are rebuilt, whatever the order
     * of the entities in the text. Every loaded entity gets its Uuid.
     *
     * Must not be called inside ECS::Registry::runDeferred (the Registry does not
     * let this module detect it).
     * @return false when this call adds an error (the world is left unchanged).
     */
    bool load(ECS::Registry& world, std::string_view text, Report* report = nullptr);

    /**
     * @brief serialize() to a file.
     *
     * The text is written to a temporary file next to the target (a name of
     * its own, so two saves at once do not meet), flushed to the disk, then
     * renamed over the target: a crash leaves the previous file or the new
     * one, never half of it. On failure the target is untouched.
     */
    bool saveFile(ECS::Registry& world, const std::filesystem::path& path, Report* report = nullptr,
                  const SaveOptions& options = {});

    /** @brief load() from a file. */
    bool loadFile(ECS::Registry& world, const std::filesystem::path& path, Report* report = nullptr);

    // --- One value ----------------------------------------------------------

    /**
     * @brief Writes one reflected value (a component, a field, an element).
     *
     * There is no world to resolve entity references: they are written as
     * null, with a warning.
     * @return The text, or an empty string with an error in the report.
     */
    [[nodiscard]] std::string serialize(ECS::reflect::Ref value, Report* report = nullptr);

    /**
     * @brief Reads text into an existing value.
     *
     * A struct field absent from the text keeps its current value. A
     * container (vector, map...) takes exactly the content of the text: it is
     * resized, or emptied then filled. Entity references cannot be resolved
     * and are skipped with a warning.
     * @return false when the text is malformed or the Ref is invalid.
     */
    bool load(ECS::reflect::Ref target, std::string_view text, Report* report = nullptr);
}
