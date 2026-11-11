// The demo scene's renderer: the view it returns stays put while an object
// is dragged in it, and follows an object that leaves it.

#include <boost/ut.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>

#include <cmath>

#include "core/GuiContext/GuiContext.hpp"
#include "scene/DemoScene/DemoScene.hpp"
#include "scene/DemoSceneRenderer/DemoSceneRenderer.hpp"
#include "scene/Selection.hpp"

using namespace boost::ut;

namespace {
    constexpr ImVec2 AreaMin(0.0f, 0.0f);
    constexpr ImVec2 AreaMax(800.0f, 600.0f);

    bool inArea(ImVec2 point)
    {
        return point.x >= AreaMin.x && point.x <= AreaMax.x && point.y >= AreaMin.y && point.y <= AreaMax.y;
    }
}

void run_demo_scene_renderer_tests()
{
    // Set for every window: SDL_Quit() clears the hints.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");

    "an object moved under the mouse is drawn under it"_test = [] {
        GuiContext gui("tests_editor", 800, 600);
        DemoScene scene;
        const HierarchyModel::ObjectId id = scene.create(HierarchyModel::InvalidId, "Only object");
        Selection selection;
        DemoSceneRenderer renderer(scene, selection);
        gui.beginFrame();
        ImDrawList *drawList = ImGui::GetBackgroundDrawList();

        const ViewportView first = renderer.draw(drawList, AreaMin, AreaMax);
        // As the Viewport's mouse does: 100 pixels to the right, back into the world.
        const ImVec2 start = first.toScreen(Vec2{0.0f, 0.0f});
        const ImVec2 mouse(start.x + 100.0f, start.y);
        scene.set(id, "Transform", "position", first.toWorld(mouse));
        const ViewportView next = renderer.draw(drawList, AreaMin, AreaMax);

        const ImVec2 drawn = next.toScreen(first.toWorld(mouse));
        expect(std::abs(drawn.x - mouse.x) < 0.5f && std::abs(drawn.y - mouse.y) < 0.5f) << "the view did not move under it";
        gui.present();
    };

    "an object moved out of the view is brought back into it"_test = [] {
        GuiContext gui("tests_editor", 800, 600);
        DemoScene scene;
        const HierarchyModel::ObjectId id = scene.create(HierarchyModel::InvalidId, "Only object");
        Selection selection;
        DemoSceneRenderer renderer(scene, selection);
        gui.beginFrame();
        ImDrawList *drawList = ImGui::GetBackgroundDrawList();

        renderer.draw(drawList, AreaMin, AreaMax);
        scene.set(id, "Transform", "position", Vec2{100000.0f, 0.0f});
        const ViewportView next = renderer.draw(drawList, AreaMin, AreaMax);

        expect(inArea(next.toScreen(Vec2{100000.0f, 0.0f}))) << "the view was fitted again";
        gui.present();
    };
}
