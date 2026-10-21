/*
    Created on: 30/03/2026
    Filename: main.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#include "src/GuiContext.hpp"
#include "src/components/Button.hpp"
#include "src/components/NumberInput.hpp"
#include "src/components/TextInput.hpp"

#include <imgui.h>

#include <exception>
#include <iostream>

int main()
{
    try {
        GuiContext gui("GUI", 1280, 720);

        Button helloButton("Say hello", [] { std::cout << "Hello!" << std::endl; });
        Button cancelButton("Cancel", [] { std::cout << "Cancel clicked" << std::endl; }, Button::Variant::Secondary);
        Button quitButton("Quit", [] { std::cout << "Quit clicked" << std::endl; }, Button::Variant::Danger,
            ImVec2(120, 0));

        TextInput nameInput("Name", "Enter your name...",
            [](const std::string &value) { std::cout << "Name: " << value << std::endl; });
        nameInput.setOnSubmit([](const std::string &value) { std::cout << "Name submitted: " << value << std::endl; });
        TextInput passwordInput("Password", "Secret");
        passwordInput.setPassword(true);

        NumberInput<int> ageInput("Age", 18, 1, [](int value) { std::cout << "Age: " << value << std::endl; });
        ageInput.setRange(0, 120);
        NumberInput<float> speedInput("Speed", 1.0f, 0.1f,
            [](float value) { std::cout << "Speed: " << value << std::endl; });
        speedInput.setRange(0.0f, 10.0f);

        while (gui.isRunning()) {
            gui.pollEvents();
            gui.beginFrame();
            // ImGui::ShowDemoWindow(); // ImGui demo window is disabled by default, uncomment the line above to enable it

            ImGui::Begin("Buttons");
            helloButton.render();
            ImGui::SameLine();
            cancelButton.render();
            ImGui::SameLine();
            quitButton.render();
            ImGui::End();

            ImGui::Begin("Inputs");
            nameInput.render();
            passwordInput.render();
            ageInput.render();
            speedInput.render();
            ImGui::End();

            gui.endFrame();
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
