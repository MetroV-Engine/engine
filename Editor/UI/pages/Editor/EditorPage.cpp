#include "EditorPage.hpp"

#include <cstring>
#include <imgui.h>

#include "../../hooks/UseWindowLayout.hpp"

namespace Editor::UI::Pages {

    EditorPage::EditorPage()  = default;
    EditorPage::~EditorPage() = default;

    void EditorPage::addPanel(std::shared_ptr<Components::Panel> panel)
    {
        _panels.emplace_back(std::move(panel));
    }

    void EditorPage::renderDockspace(float x, float y, float w, float h)
    {
        ImGui::SetNextWindowPos(ImVec2(x, y));
        ImGui::SetNextWindowSize(ImVec2(w, h));

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace", nullptr, flags);
        ImGui::PopStyleVar(3);

        const ImGuiID dockId = ImGui::GetID("ViewportDockSpace");
        ImGui::DockSpace(dockId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::End();
    }

    void EditorPage::renderPanel(const std::shared_ptr<Components::Panel> &panel,
                                 ECS::registry &reg,
                                 std::vector<ECS::System::ISystem *> &systems)
    {
        if (!panel->isVisible()) return;

        const auto L = Hooks::useWindowLayout();
        const char *name = panel->getName();

        const bool isToolbar   = std::strcmp(name, "Toolbar")   == 0;
        const bool isProject   = std::strcmp(name, "Project")   == 0;
        const bool isViewport  = std::strcmp(name, "Viewport")  == 0;
        const bool isInspector = std::strcmp(name, "Inspector") == 0;
        const bool fixed = isToolbar || isProject || isViewport || isInspector;
        const ImGuiCond cond = fixed ? ImGuiCond_Always : ImGuiCond_FirstUseEver;

        if (isToolbar) {
            ImGui::SetNextWindowPos(ImVec2(0, 0), cond);
            ImGui::SetNextWindowSize(ImVec2(L.winWidth, L.toolbarHeight), cond);
        } else if (isProject) {
            ImGui::SetNextWindowPos(ImVec2(0, L.toolbarHeight), cond);
            ImGui::SetNextWindowSize(ImVec2(L.projectWidth, L.contentHeight), cond);
        } else if (isViewport) {
            ImGui::SetNextWindowPos(ImVec2(L.projectWidth, L.toolbarHeight), cond);
            ImGui::SetNextWindowSize(ImVec2(L.viewportWidth, L.contentHeight), cond);
        } else if (isInspector) {
            ImGui::SetNextWindowPos(ImVec2(L.winWidth - L.inspectorWidth, L.toolbarHeight), cond);
            ImGui::SetNextWindowSize(ImVec2(L.inspectorWidth, L.contentHeight), cond);
        }

        ImGuiWindowFlags flags = panel->getExtraFlags();
        if (!panel->isOverlay())
            flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        if (ImGui::Begin(name, nullptr, flags)) {
            panel->update(reg);
            panel->render(systems);
        }
        ImGui::End();
    }

    void EditorPage::render(ECS::registry &reg,
                            std::vector<ECS::System::ISystem *> &systems)
    {
        const auto L = Hooks::useWindowLayout();
        renderDockspace(L.projectWidth, L.toolbarHeight, L.viewportWidth, L.contentHeight);

        // Pass 1 — base panels
        for (const auto &p : _panels) {
            if (!p->isOverlay()) renderPanel(p, reg, systems);
        }
        // Pass 2 — overlays
        for (const auto &p : _panels) {
            if (p->isOverlay() && p->isVisible()) renderPanel(p, reg, systems);
        }
    }

}
