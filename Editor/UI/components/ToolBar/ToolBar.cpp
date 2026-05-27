#include "ToolBar.hpp"

#include <imgui.h>

#include "../../pages/Editor/EditorPage.hpp"

namespace Editor::UI::Components {

    ToolBar::ToolBar(Pages::EditorPage *page) : _page(page) {}

    int ToolBar::getExtraFlags() const
    {
        return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
               ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar;
    }

    void ToolBar::update(ECS::registry &reg) { _reg = &reg; }

    void ToolBar::render(std::vector<ECS::System::ISystem *> systems)
    {
        (void)systems;
        if (!_reg || !_page) return;
        _playButton.render(*_reg, _page->enabledSystems());
    }

}
