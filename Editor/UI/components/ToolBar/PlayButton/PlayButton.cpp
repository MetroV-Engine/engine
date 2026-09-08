#include "PlayButton.hpp"

#include <algorithm>
#include <imgui.h>

#include "GameStructure/Core/Core.hpp"
#include "Scripting/ScriptEngine.hpp"

#include "../../common/Button/Button.hpp"
#include "../../../providers/Scale/ScaleProvider.hpp"
#include "../../../providers/Theme/ThemeProvider.hpp"

namespace Editor::UI::Components {

    void PlayButton::startGame(ECS::registry &reg,
                               const std::map<ECS::System::ISystem *, bool> &enabledSystems)
    {
        _snapshot.capture(reg);
        _applied.clear();
        for (const auto &[sys, on] : enabledSystems) {
            if (sys && on) _applied.push_back(sys);
        }

        Game::Core::getInstance().createWindow();
        Editor::Scripting::ScriptEngine::getInstance().setActiveRegistry(&reg);
        _gameRunning = true;
    }

    void PlayButton::stopGame(ECS::registry &reg)
    {
        Game::Core::getInstance().destroyWindow();
        Editor::Scripting::ScriptEngine::getInstance().resetInstances();
        _snapshot.restore(reg);
        _gameRunning = false;
    }

    void PlayButton::tickGame(ECS::registry &reg,
                              const std::map<ECS::System::ISystem *, bool> &enabledSystems)
    {
        (void)enabledSystems;
        if (!_gameRunning) return;

        if (!Game::Core::getInstance().tick(reg, _applied)) {
            stopGame(reg);
        }
    }

    void PlayButton::render(ECS::registry &reg,
                            const std::map<ECS::System::ISystem *, bool> &enabledSystems)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
            if (_gameRunning) stopGame(reg);
            else              startGame(reg, enabledSystems);
        }

        const auto scale = Providers::useScale().getScale();
        const auto &d    = Providers::useDesign();

        const float buttonW = std::max(d.minPlayButtonW, d.playButtonWidth  * scale.x);
        const float buttonH = std::max(d.minPlayButtonH, d.playButtonHeight * scale.y);
        const float pad     = std::max(5.0f, d.buttonPadding * scale.x);

        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - buttonW - pad,
                                   (ImGui::GetWindowHeight() - buttonH) * 0.5f));

        const ButtonProps props {
            .variant = _gameRunning ? ButtonVariant::Danger : ButtonVariant::Success,
            .size    = ImVec2(buttonW, buttonH),
        };

        if (Button::render(_gameRunning ? "Stop" : "F5", props)) {
            if (_gameRunning) stopGame(reg);
            else              startGame(reg, enabledSystems);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", _gameRunning ? "Stop the game (F5)" : "Start the game (F5)");
            ImGui::EndTooltip();
        }
    }

}
