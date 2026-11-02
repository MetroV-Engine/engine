#pragma once

#include <string>

namespace ECS {
    /**
     * @brief Optional display name of an entity, stored as a regular component.
     *
     * Added by spawnEntity(name) when the name is not empty; it can then be
     * read, replaced or removed like any component. Entities without one
     * (most of them: tiles, bullets...) cost nothing; tools show them as
     * "Entity <index>" (see reflect::RegistryInspector::displayName).
     */
    struct Name {
        std::string value;
    };
}
