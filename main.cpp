/*
    Filename: main.cpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include <iostream>

#include "UI/App.hpp"
#include "GameStructure/Components/ComponentsInit.hpp"
#include "Editor/Save/SaveManager/SaveManager.hpp"

int main() {
    try {
        ECS::Component::initializeComponents();
        Editor::SaveManager &saveManager = Editor::SaveManager::getInstance();
        saveManager.setSaveFile(std::string(PROJECT_ROOT_DIR) + "/scene.json");

        Editor::UI::App::getInstance().run();
    } catch (const std::exception &e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
