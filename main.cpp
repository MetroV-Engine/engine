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
#include "src/components/Checkbox.hpp"
#include "src/components/ComboBox.hpp"
#include "src/components/NumberInput.hpp"
#include "src/components/PerformanceOverlay.hpp"
#include "src/components/RadioGroup.hpp"
#include "src/components/TextInput.hpp"

#include <imgui.h>

#include <exception>
#include <iostream>
#include <utility>
#include <vector>

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

        Checkbox vsyncCheckbox("VSync", true, [&gui](bool checked) {
            std::cout << "VSync: " << (checked ? "on" : "off") << std::endl;
            gui.setVSync(checked);
        });
        gui.setVSync(vsyncCheckbox.isChecked());
        Checkbox fullscreenCheckbox("Fullscreen", false, [&gui](bool checked) {
            std::cout << "Fullscreen: " << (checked ? "on" : "off") << std::endl;
            gui.setFullscreen(checked);
        });

        const std::vector<std::pair<int, int>> resolutions = {{1280, 720}, {1920, 1080}, {2560, 1440}};
        ComboBox resolutionCombo("Resolution", {"1280x720", "1920x1080", "2560x1440"}, 0,
            [&gui, &resolutions](int index, const std::string &option) {
                std::cout << "Resolution: " << option << std::endl;
                gui.setWindowSize(resolutions[index].first, resolutions[index].second);
            });
        RadioGroup qualityRadio("Quality", {"Low", "Medium", "High"}, 1,
            [](int, const std::string &option) { std::cout << "Quality: " << option << std::endl; });
        qualityRadio.setHorizontal(true);

        PerformanceOverlay performanceOverlay(gui.timer());
        Checkbox overlayCheckbox("Show performance", true,
            [&performanceOverlay](bool checked) { performanceOverlay.setVisible(checked); });

        gui.run([&] {
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

            ImGui::Begin("Selection");
            vsyncCheckbox.render();
            fullscreenCheckbox.render();
            resolutionCombo.render();
            qualityRadio.render();
            overlayCheckbox.render();
            ImGui::End();

            performanceOverlay.render();

        });
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
