/*
    Created on: 09/03/2026
    Filename: SaveManager.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_SAVEMANAGER_HPP
#define PROTO_SAVEMANAGER_HPP
#include "../json.hpp"
#include "../../ECS/Include/Registry.hpp"

using json = nlohmann::json;

namespace Editor
{
    class SaveManager {
        public:
            static SaveManager& getInstance() {
                static SaveManager instance; // Guaranteed to be destroyed and instantiated on first use.
                return instance;
            }

            // Delete copy constructor and assignment operator to prevent copying
            SaveManager(const SaveManager&) = delete;
            SaveManager& operator=(const SaveManager&) = delete;

            std::string getSaveFile() const { return _saveFile; }
            void setSaveFile(const std::string& filename) { _saveFile = filename; }

            void load(ECS::registry &reg);
            void save(ECS::registry &reg);
        private:
            SaveManager() = default;
            ~SaveManager() = default;

            std::string _saveFile = "";
            json _j;
    };
} // Editor

#endif //PROTO_SAVEMANAGER_HPP
