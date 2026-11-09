// The real Editor on an ECS world, without a window on screen: a simulated
// mouse drags entity templates from the Entity Palette, one event every few
// frames. Run on macOS with:
//   SDL_VIDEO_DRIVER=dummy SDL_RENDER_DRIVER=software ./build/src/Editor/tests_editor

#include <boost/ut.hpp>

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/Logs.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "core/Editor/Editor.hpp"
#include "core/GuiContext/GuiContext.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"

using namespace boost::ut;

// Prefixed with "Drop": the TypeRegistry is process-wide.
namespace {
    struct DropHealth { int value = 10; };

    enum class PlayMoment { Never, BeforeDrag, DuringDrag };

    struct DragScenario
    {
        int templateIndex = 0;
        // Name of the window the mouse is released over.
        std::string target;
        PlayMoment play = PlayMoment::Never;
    };

    struct DragResult
    {
        bool draggingBeforeRelease = false;
        bool draggingAfterPlay = false;
        int frames = 0;
        std::string selectedName;
        std::vector<std::string> selectedComponents;
    };

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
            return [this, &selection](ImDrawList *, ImVec2, ImVec2) { step(selection); };
        }

    private:
        GuiContext &_gui;
        DragScenario _scenario;
        DragResult &_result;
        CommandRegistry *_commands = nullptr;
        ImVec2 _grab;

        void step(const Selection &selection)
        {
            const int frame = ++_result.frames;
            switch (frame) {
            case 5:
                if (_scenario.play == PlayMoment::BeforeDrag)
                    _commands->execute("play");
                break;
            case 10:
                _grab = paletteItem(_scenario.templateIndex);
                moveMouse(_grab);
                break;
            case 13: setMouseDown(true); break;
            case 16: moveMouse(ImVec2(_grab.x + 25.0f, _grab.y + 2.0f)); break;
            case 19: moveMouse(windowCenter(_scenario.target)); break;
            case 22:
                _result.draggingBeforeRelease = isDraggingTemplate();
                if (_scenario.play == PlayMoment::DuringDrag)
                    _commands->execute("play");
                break;
            case 28:
                _result.draggingAfterPlay = isDraggingTemplate();
                setMouseDown(false);
                break;
            case 34:
                if (selection.has()) {
                    _result.selectedName = hierarchy().name(selection.get());
                    for (const ComponentInfo &component : properties().components(selection.get()))
                        _result.selectedComponents.push_back(component.name);
                }
                break;
            case 36: _gui.stop(); break;
            default: break;
            }
        }
    };

    // The editor saves its layout in the working directory: each run gets a
    // fresh one, removed afterwards.
    class FreshDirectory
    {
    public:
        FreshDirectory()
            : _previous(std::filesystem::current_path()),
              _path(std::filesystem::temp_directory_path()
                    / ("metrov-editor-drop-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
        {
            std::filesystem::create_directories(_path);
            std::filesystem::current_path(_path);
        }
        ~FreshDirectory()
        {
            std::filesystem::current_path(_previous);
            std::error_code ignored;
            std::filesystem::remove_all(_path, ignored);
        }
        FreshDirectory(const FreshDirectory &) = delete;
        FreshDirectory &operator=(const FreshDirectory &) = delete;

    private:
        std::filesystem::path _previous;
        std::filesystem::path _path;
    };

    DragResult runDrag(ECS::Registry &world, DragScenario scenario)
    {
        FreshDirectory directory;
        // Set for every run: SDL_Quit(), when the previous editor closed, clears
        // the hints. The SDL_VIDEO_DRIVER environment variable takes precedence.
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        ECS::SystemManager systems;
        GuiContext gui("tests_editor", 1280, 720);
        DragResult result;
        {
            Editor editor(gui, std::make_unique<DragBackend>(world, systems, gui, std::move(scenario), result));
            editor.addEntityTemplate({"Drop Mob", "Mob", {"DropHealth"}});
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

void run_entity_drag_drop_tests()
{
    // Template 0 is the scene's "Empty Entity"; template 1 is "Drop Mob".
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
        expect(!result.draggingAfterPlay) << "PayloadAutoExpire cancelled it";
        expect(world.entityCount() == 0_ul);
        expect(result.selectedName.empty());
    };

    "a template cannot be dragged during Play"_test = [] {
        ECS::Registry world;

        const DragResult result = runDrag(world, {.templateIndex = 1, .target = "Viewport", .play = PlayMoment::BeforeDrag});

        expect(!result.draggingBeforeRelease);
        expect(world.entityCount() == 0_ul);
        expect(result.selectedName.empty());
    };
}
