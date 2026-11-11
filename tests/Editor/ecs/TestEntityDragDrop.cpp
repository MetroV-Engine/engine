// The real Editor on an ECS world, without a window on screen: a simulated
// mouse drags entity templates from the Entity Palette and moves objects in
// the Viewport, one event every few frames. Run on macOS with:
//   SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software ./build/tests/tests_editor

#include <boost/ut.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/FreshWorkingDirectory.hpp"
#include "Support/Logs.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"

using namespace boost::ut;

// Prefixed with "Drop": the TypeRegistry is process-wide.
namespace {
    struct DropHealth { int value = 10; };
    struct DropVector { float x = 0, y = 0; };
    // The component and field the mouse writes: the name matters here.
    struct Transform { DropVector position; };

    // Pixels of the Viewport per world unit, in these tests.
    constexpr float ViewScale = 2.0f;

    enum class PlayMoment { Never, BeforeDrag, DuringDrag, OnRelease };

    struct DragScenario
    {
        int templateIndex = 0;
        // Name of the window the mouse is released over.
        std::string target;
        PlayMoment play = PlayMoment::Never;
        // If set, the mouse drags inside the Viewport instead of from the
        // palette: from this world position to moveTo.
        std::optional<Vec2> moveFrom = std::nullopt;
        Vec2 moveTo{};
    };

    struct DragResult
    {
        bool draggingBeforeRelease = false;
        bool draggingAtRelease = false;
        int frames = 0;
        std::string selectedName;
        std::vector<std::string> selectedComponents;
        std::optional<Vec2> selectedPosition;
        // World position under the mouse where it was released.
        Vec2 releaseWorld{};
    };

    bool near(Vec2 a, Vec2 b)
    {
        // ImGui rounds the mouse position down to a pixel.
        const float tolerance = 1.0f / ViewScale + 0.01f;
        return std::abs(a.x - b.x) <= tolerance && std::abs(a.y - b.y) <= tolerance;
    }

    bool isDraggingTemplate()
    {
        const ImGuiPayload *payload = ImGui::GetDragDropPayload();
        return payload && payload->IsDataType(EntityTemplatePayloadType);
    }

    ImVec2 windowCenter(const std::string &name)
    {
        return ImGui::FindWindowByName(name.c_str())->InnerRect.GetCenter();
    }

    ImVec2 paletteItem(int index)
    {
        const ImGuiWindow *palette = ImGui::FindWindowByName("Entity Palette");
        return ImVec2(palette->InnerRect.Min.x + 30.0f,
            palette->InnerRect.Min.y + ImGui::GetStyle().WindowPadding.y
                + ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(index) + ImGui::GetTextLineHeight() * 0.5f);
    }

    void moveMouse(ImVec2 position)
    {
        ImGui::GetIO().AddMousePosEvent(position.x, position.y);
    }

    void setMouseDown(bool down)
    {
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, down);
    }

    // EcsBackend whose Viewport renderer, called once per frame, moves the mouse.
    class DragBackend : public EcsBackend
    {
    public:
        DragBackend(ECS::Registry &world, ECS::SystemManager &systems, GuiContext &gui, DragScenario scenario, DragResult &result)
            : EcsBackend(world, systems), _gui(gui), _scenario(std::move(scenario)), _result(result)
        {
        }

        void registerCommands(CommandRegistry &commands) override
        {
            EcsBackend::registerCommands(commands);
            _commands = &commands;
        }

        Renderer renderer(const Selection &selection) override
        {
            return [this, &selection](ImDrawList *, ImVec2 min, ImVec2) {
                _view = ViewportView{min, ViewScale};
                step(selection);
                return _view;
            };
        }

    private:
        GuiContext &_gui;
        DragScenario _scenario;
        DragResult &_result;
        CommandRegistry *_commands = nullptr;
        ImVec2 _grab;
        ViewportView _view;

        ImVec2 mouseStart() const
        {
            return _scenario.moveFrom ? _view.toScreen(*_scenario.moveFrom) : paletteItem(_scenario.templateIndex);
        }

        ImVec2 mouseEnd() const
        {
            return _scenario.moveFrom ? _view.toScreen(_scenario.moveTo) : windowCenter(_scenario.target);
        }

        void step(const Selection &selection)
        {
            const int frame = ++_result.frames;
            switch (frame) {
            case 5:
                if (_scenario.play == PlayMoment::BeforeDrag)
                    _commands->execute("play");
                break;
            case 10:
                _grab = mouseStart();
                moveMouse(_grab);
                break;
            case 13: setMouseDown(true); break;
            case 16: moveMouse(ImVec2(_grab.x + 25.0f, _grab.y + 2.0f)); break;
            case 19:
                moveMouse(mouseEnd());
                _result.releaseWorld = _view.toWorld(mouseEnd());
                break;
            case 22:
                _result.draggingBeforeRelease = isDraggingTemplate();
                if (_scenario.play == PlayMoment::DuringDrag)
                    _commands->execute("play");
                break;
            case 28:
                _result.draggingAtRelease = isDraggingTemplate();
                if (_scenario.play == PlayMoment::OnRelease)
                    _commands->execute("play");
                setMouseDown(false);
                break;
            case 34:
                if (selection.has()) {
                    _result.selectedName = hierarchy().name(selection.get());
                    for (const ComponentInfo &component : properties().components(selection.get()))
                        _result.selectedComponents.push_back(component.name);
                    _result.selectedPosition = properties().getAs<Vec2>(selection.get(), "Transform", "position");
                }
                break;
            case 36: _gui.stop(); break;
            default: break;
            }
        }
    };

    DragResult runDrag(ECS::Registry &world, DragScenario scenario)
    {
        FreshWorkingDirectory directory;
        // Set for every run: SDL_Quit(), when the previous editor closed, clears
        // the hints. The SDL_VIDEO_DRIVER environment variable takes precedence.
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        ECS::SystemManager systems;
        GuiContext gui("tests_editor", 1280, 720);
        DragResult result;
        {
            Editor editor(gui, std::make_unique<DragBackend>(world, systems, gui, std::move(scenario), result));
            // Templates 1 and 2; template 0 is the scene's "Empty Entity".
            editor.addEntityTemplate({"Drop Mob", "Mob", {"DropHealth"}});
            editor.addEntityTemplate({"Placed", "Placed", {"Transform"}});
            editor.run();
        }
        return result;
    }

    bool has(const std::vector<std::string> &names, const std::string &name)
    {
        return std::ranges::find(names, name) != names.end();
    }
}

E_REFLECT(DropHealth) E_FIELD(value, int) E_END
E_REFLECT(DropVector) E_FIELD(x, float) E_FIELD(y, float) E_END
E_REFLECT(Transform) E_FIELD(position, DropVector) E_END

void run_entity_drag_drop_tests()
{
    "dropping a template on the Viewport places its entity under the mouse"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 2, .target = "Viewport"});

        expect(result.selectedName == std::string("Placed"));
        expect(result.selectedPosition && near(*result.selectedPosition, result.releaseWorld));
        expect(result.selectedPosition && result.selectedPosition->x > 1.0f) << "not left at the origin";
    };

    "dragging an object in the Viewport selects and moves it"_test = [] {
        ECS::Registry world;
        const ECS::Entity moved = world.spawnEntity("Moved");
        world.addComponent(moved, Transform{{100, 80}});

        const DragResult result = runDrag(world, {.moveFrom = Vec2{100, 80}, .moveTo = Vec2{220, 150}});

        expect(result.selectedName == std::string("Moved"));
        const Transform &transform = world.getComponent<Transform>(moved);
        expect(near(Vec2{transform.position.x, transform.position.y}, Vec2{220, 150}));
    };

    "clicking away from every object moves nothing"_test = [] {
        ECS::Registry world;
        const ECS::Entity still = world.spawnEntity("Still");
        world.addComponent(still, Transform{{100, 80}});

        const DragResult result = runDrag(world, {.moveFrom = Vec2{10, 10}, .moveTo = Vec2{220, 150}});

        expect(result.selectedName.empty());
        expect(world.getComponent<Transform>(still).position.x == 100.0f);
    };

    "objects cannot be moved during Play"_test = [] {
        ECS::Registry world;
        const ECS::Entity still = world.spawnEntity("Still");
        world.addComponent(still, Transform{{100, 80}});

        const DragResult result = runDrag(world, {.play = PlayMoment::BeforeDrag, .moveFrom = Vec2{100, 80}, .moveTo = Vec2{220, 150}});

        expect(result.selectedName.empty()) << "the click did not grab it";
        expect(world.getComponent<Transform>(still).position.x == 100.0f);
    };

    "dropping a template on the Hierarchy creates its entity and selects it"_test = [] {
        ECS::Registry world;
        world.spawnEntity("A");
        Logger::instance().clear();

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Hierarchy"});

        expect(result.frames >= 36_i);
        expect(result.draggingBeforeRelease) << "the palette item was really dragged";
        expect(world.entityCount() == 2_ul);
        expect(result.selectedName == std::string("Mob"));
        expect(has(result.selectedComponents, "DropHealth")) << "the template's components were added";
        expect(loggedErrors() == 0_ul);
    };

    "dropping a template on the Viewport creates its entity and selects it"_test = [] {
        ECS::Registry world;
        Logger::instance().clear();

        const DragResult result = runDrag(world, {.templateIndex = 0, .target = "Viewport"});

        expect(result.draggingBeforeRelease);
        expect(world.entityCount() == 1_ul);
        expect(result.selectedName == std::string("Entity"));
        expect(result.selectedComponents.empty());
        expect(loggedErrors() == 0_ul);
    };

    "releasing a template outside a drop target creates nothing"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Inspector"});

        expect(result.draggingBeforeRelease);
        expect(world.entityCount() == 0_ul);
        expect(result.selectedName.empty());
    };

    "pressing Play during the drag cancels it"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Hierarchy", .play = PlayMoment::DuringDrag});

        expect(result.draggingBeforeRelease) << "the drag had started before Play";
        expect(!result.draggingAtRelease) << "PayloadAutoExpire cancelled it";
        expect(world.entityCount() == 0_ul);
        expect(result.selectedName.empty());
    };

    "pressing Play at the moment of the drop creates nothing"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Hierarchy", .play = PlayMoment::OnRelease});

        expect(result.draggingAtRelease) << "the payload was still active";
        expect(result.selectedName.empty()) << "the drop was refused";
        expect(world.entityCount() == 0_ul);
    };

    "a template cannot be dragged during Play"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Viewport", .play = PlayMoment::BeforeDrag});

        expect(!result.draggingBeforeRelease);
        expect(world.entityCount() == 0_ul);
        expect(result.selectedName.empty());
    };
}
