/*
    Filename: Viewport.hpp
    Description: 2D scene viewport with camera, drag, scale handles and grid.
                 Replaces the old InGameRendering layer.

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_VIEWPORT_HPP
#define PROTO_UI_VIEWPORT_HPP

#include <imgui.h>

#include "Entity/Base/Drawable/Drawable.hpp"
#include "Entity/Base/Position/Position.hpp"
#include "Entity/Base/Bounds/Bounds.hpp"
#include "Zipper.hpp"

#include "../common/Panel/Panel.hpp"

namespace Editor::UI::Components {

    class Viewport : public Panel {
    public:
        Viewport();
        ~Viewport() override = default;

        void update(ECS::registry &reg) override;
        void render(std::vector<ECS::System::ISystem *> systems) override;

        [[nodiscard]] int getExtraFlags() const override;
        [[nodiscard]] const char *getName() const override { return "Viewport"; }

    private:
        struct ViewportContext {
            ImDrawList *drawList = nullptr;
            ImVec2 origin = {0.f, 0.f};
            ImVec2 size   = {0.f, 0.f};
            ImVec2 end    = {0.f, 0.f};
            bool   hovered = false;
        };

        struct DragState {
            bool active = false;
            std::size_t entityId = static_cast<std::size_t>(-1);
            ImVec2 offset = {0.f, 0.f};
        };

        enum class ScaleHandle { None, TopLeft, TopRight, BottomLeft, BottomRight };

        struct ScaleState {
            bool selecting = false;
            bool dragging  = false;
            std::size_t entityId = static_cast<std::size_t>(-1);
            ScaleHandle handle = ScaleHandle::None;

            ImVec2 startMouseWorld{};
            float startX = 0.f;
            float startY = 0.f;
            float startW = 0.f;
            float startH = 0.f;
        };

        bool fetchComponents();
        bool setupViewport(ViewportContext &ctx);
        void handleZoom(const ViewportContext &ctx, const ImGuiIO &io);
        void handleCameraPan(const ViewportContext &ctx, const ImGuiIO &io);
        void handleEntityDrag(const ViewportContext &ctx, const ImGuiIO &io);
        void drawBackground(const ViewportContext &ctx) const;
        void drawGrid(const ViewportContext &ctx) const;
        void drawEntities(const ViewportContext &ctx) const;
        void drawScreenBounds(const ViewportContext &ctx) const;
        void handleScreenResize(const ViewportContext &ctx, const ImGuiIO &io);

        ImVec2 worldToScreen(const ViewportContext &ctx, float wx, float wy) const;
        ImVec2 screenToWorld(const ViewportContext &ctx, float sx, float sy) const;

        std::size_t findEntityUnderMouse(const ImVec2 &mouseWorld) const;
        ScaleHandle getHoveredScaleHandle(const ViewportContext &ctx, std::size_t entityId, const ImVec2 &mouseScreen) const;
        void drawScaleHandles(const ViewportContext &ctx) const;
        void handleEntityScale(const ViewportContext &ctx, const ImGuiIO &io);

        ECS::registry *_reg = nullptr;
        ECS::sparse_set<ECS::Entity::Position> *_positions = nullptr;
        ECS::sparse_set<ECS::Entity::Drawable> *_drawables = nullptr;
        ECS::sparse_set<ECS::Entity::Bounds>   *_bounds    = nullptr;

        ImVec2 _camera = {0.0f, 0.0f};
        float  _zoom   = 1.0f;

        bool   _panning = false;
        ImVec2 _panStartMouse  = {0.f, 0.f};
        ImVec2 _panStartCamera = {0.f, 0.f};

        DragState  _drag;
        ScaleState _scale;

        bool   _screenResizing = false;
        ImVec2 _screenResizeStart = {0.f, 0.f};
        float  _screenStartW = 0.f;
        float  _screenStartH = 0.f;
    };

}

#endif
