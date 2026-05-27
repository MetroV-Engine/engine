/*
    Created on: 2/23/26
    Filename: InGameRendering.cpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#include "Viewport.hpp"
#include <imgui.h>
#include <iostream>
#include <cmath>

#include "../../providers/Selection/SelectionProvider.hpp"
#include "ProjectContext.hpp"

namespace Editor::UI::Components {
    Viewport::Viewport()
    {
    }
    
    int Viewport::getExtraFlags() const
    {
        return ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking |
               ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    void Viewport::render(std::vector<ECS::System::ISystem *> systems)
    {
        if (!_reg)
            return;
        
        fetchComponents();

        ViewportContext ctx;
        if (!setupViewport(ctx))
            return;

        ImGuiIO& io = ImGui::GetIO();

        handleZoom(ctx, io);
        handleScreenResize(ctx, io);
        handleEntityDrag(ctx, io);
        handleEntityScale(ctx, io);
        handleCameraPan(ctx, io);

        // Click on background to deselect (only when not in E mode)
        if (ctx.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
            !io.KeyShift && !io.KeyCtrl && !ImGui::IsKeyDown(ImGuiKey_E) && !_scale.selecting) {
            ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
            
            bool hitEntity = false;
            for (auto [d, p, b, entity] : zipper(*_drawables, *_positions, *_bounds)) {
                const float left = p._x;
                const float top = p._y;
                const float right = p._x + b._width;
                const float bottom = p._y + b._height;

                if (mouseWorld.x >= left && mouseWorld.x <= right &&
                    mouseWorld.y >= top  && mouseWorld.y <= bottom) {
                    hitEntity = true;
                    break;
                }
            }
            
            if (!hitEntity) {
                Editor::UI::Providers::useSelection().clearSelection();
            }
        }

        // Exit resize mode when clicking elsewhere without E key
        if (_scale.selecting && ctx.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
            !ImGui::IsKeyDown(ImGuiKey_E) && !_scale.dragging) {
            ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
            std::size_t entity = findEntityUnderMouse(mouseWorld);
            
            if (entity == static_cast<std::size_t>(-1) || entity != _scale.entityId) {
                _scale.selecting = false;
                _scale.entityId = static_cast<std::size_t>(-1);
                _scale.handle = ScaleHandle::None;
                Editor::UI::Providers::useSelection().clearSelection();
            }
        }

        ctx.drawList->PushClipRect(ctx.origin, ctx.end, true);
        drawBackground(ctx);
        drawScreenBounds(ctx);
        drawGrid(ctx);
        drawEntities(ctx);
        drawScaleHandles(ctx);
        ctx.drawList->PopClipRect();
    }


    bool Viewport::fetchComponents()
    {
        _positions = _reg->get_if<ECS::Entity::Position>();
        _drawables = _reg->get_if<ECS::Entity::Drawable>();
        _bounds = _reg->get_if<ECS::Entity::Bounds>();

        return _positions || _drawables || _bounds;
    }

    bool Viewport::setupViewport(ViewportContext& ctx)
    {
        ctx.drawList = ImGui::GetWindowDrawList();
        ctx.origin = ImGui::GetCursorScreenPos();
        ctx.size = ImGui::GetContentRegionAvail();

        if (ctx.size.x <= 0.f || ctx.size.y <= 0.f)
            return false;

        ctx.end = ImVec2(ctx.origin.x + ctx.size.x, ctx.origin.y + ctx.size.y);

        ImGui::InvisibleButton("##viewport", ctx.size,
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonMiddle |
            ImGuiButtonFlags_MouseButtonRight);

        ctx.hovered = ImGui::IsItemHovered();
        return true;
    }

    ImVec2 Viewport::worldToScreen(const ViewportContext& ctx, float wx, float wy) const
    {
        return ImVec2(
            ctx.origin.x + (wx - _camera.x) * _zoom,
            ctx.origin.y + (wy - _camera.y) * _zoom
        );
    }

    ImVec2 Viewport::screenToWorld(const ViewportContext& ctx, float sx, float sy) const
    {
        return ImVec2(
            _camera.x + (sx - ctx.origin.x) / _zoom,
            _camera.y + (sy - ctx.origin.y) / _zoom
        );
    }

    void Viewport::handleZoom(const ViewportContext& ctx, const ImGuiIO& io)
    {
        if (!ctx.hovered || io.MouseWheel == 0.0f)
            return;

        float zoomFactor = 1.0f + io.MouseWheel * 0.1f;
        if (zoomFactor < 0.1f)
            zoomFactor = 0.1f;

        ImVec2 mouse = io.MousePos;
        ImVec2 mouseWorldBefore = screenToWorld(ctx, mouse.x, mouse.y);

        _zoom *= zoomFactor;
        if (_zoom < 0.1f)
            _zoom = 0.1f;
        if (_zoom > 20.0f)
            _zoom = 20.0f;

        ImVec2 mouseWorldAfter = screenToWorld(ctx, mouse.x, mouse.y);

        _camera.x += mouseWorldBefore.x - mouseWorldAfter.x;
        _camera.y += mouseWorldBefore.y - mouseWorldAfter.y;
    }

    void Viewport::handleEntityDrag(const ViewportContext& ctx, const ImGuiIO& io)
    {
        if (!_positions || !_drawables || !_bounds) return;

        if (ctx.hovered && io.KeyShift && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);

            _drag.active = false;
            _drag.entityId = static_cast<std::size_t>(-1);

            for (auto [d, p, b, entity] : zipper(*_drawables, *_positions, *_bounds)) {
                const float left = p._x;
                const float top = p._y;
                const float right = p._x + b._width;
                const float bottom = p._y + b._height;

                const bool inside =
                    mouseWorld.x >= left && mouseWorld.x <= right &&
                    mouseWorld.y >= top  && mouseWorld.y <= bottom;

                if (inside) {
                    // Shift+Click selects the entity
                    ECS::Entity::entity ent(entity);
                    Editor::UI::Providers::useSelection().selectEntity(ent);
                    
                    _drag.active = true;
                    _drag.entityId = static_cast<std::size_t>(entity);
                    _drag.offset = ImVec2(mouseWorld.x - p._x, mouseWorld.y - p._y);
                    break;
                }
            }
        }

        if (!_drag.active)
            return;

        if (!io.KeyShift || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            _drag.active = false;
            _drag.entityId = static_cast<std::size_t>(-1);
            return;
        }

        // Show move cursor while dragging
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

        ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);

        if (_drag.entityId < _positions->size() && _positions->has(_drag.entityId)) {
            auto& draggedPos = _positions->get(_drag.entityId);
            draggedPos._x = mouseWorld.x - _drag.offset.x;
            draggedPos._y = mouseWorld.y - _drag.offset.y;
        }
    }

    Viewport::ScaleHandle Viewport::getHoveredScaleHandle( const ViewportContext& ctx, std::size_t entityId, const ImVec2& mouseScreen) const
    {
        if (entityId >= _positions->size() || !_positions->has(entityId) ||
            entityId >= _bounds->size() || !_bounds->has(entityId))
            return ScaleHandle::None;

        const auto& p = _positions->get(entityId);
        const auto& b = _bounds->get(entityId);

        ImVec2 tl = worldToScreen(ctx, p._x, p._y);
        ImVec2 tr = worldToScreen(ctx, p._x + b._width, p._y);
        ImVec2 bl = worldToScreen(ctx, p._x, p._y + b._height);
        ImVec2 br = worldToScreen(ctx, p._x + b._width, p._y + b._height);

        const float handleSize = 8.f;

        auto inHandle = [&](const ImVec2& h) {
            return mouseScreen.x >= h.x - handleSize &&
                mouseScreen.x <= h.x + handleSize &&
                mouseScreen.y >= h.y - handleSize &&
                mouseScreen.y <= h.y + handleSize;
        };

        if (inHandle(tl)) return ScaleHandle::TopLeft;
        if (inHandle(tr)) return ScaleHandle::TopRight;
        if (inHandle(bl)) return ScaleHandle::BottomLeft;
        if (inHandle(br)) return ScaleHandle::BottomRight;

        return ScaleHandle::None;
    }

    void Viewport::drawScaleHandles(const ViewportContext& ctx) const
    {
        if (!_scale.selecting || !_positions || !_bounds)
            return;

        if (_scale.entityId >= _positions->size() || !_positions->has(_scale.entityId) ||
            _scale.entityId >= _bounds->size() || !_bounds->has(_scale.entityId))
            return;

        const auto& p = _positions->get(_scale.entityId);
        const auto& b = _bounds->get(_scale.entityId);

        ImVec2 tl = worldToScreen(ctx, p._x, p._y);
        ImVec2 tr = worldToScreen(ctx, p._x + b._width, p._y);
        ImVec2 bl = worldToScreen(ctx, p._x, p._y + b._height);
        ImVec2 br = worldToScreen(ctx, p._x + b._width, p._y + b._height);

        const float s = 6.f;
        ImU32 col = IM_COL32(255, 200, 0, 255);

        auto drawHandle = [&](const ImVec2& h) {
            ctx.drawList->AddRectFilled(
                ImVec2(h.x - s, h.y - s),
                ImVec2(h.x + s, h.y + s),
                col
            );
        };

        ctx.drawList->AddRect(
            worldToScreen(ctx, p._x, p._y),
            worldToScreen(ctx, p._x + b._width, p._y + b._height),
            IM_COL32(255, 200, 0, 255),
            0.f, 0, 2.f
        );

        drawHandle(tl);
        drawHandle(tr);
        drawHandle(bl);
        drawHandle(br);
    }

    void Viewport::handleEntityScale(const ViewportContext& ctx, const ImGuiIO& io)
    {
        if (!_positions || !_bounds) return;

        const std::size_t invalid = static_cast<std::size_t>(-1);

        if (ctx.hovered && ImGui::IsKeyDown(ImGuiKey_E) && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !_scale.dragging) {
            ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
            std::size_t entity = findEntityUnderMouse(mouseWorld);

            if (entity != invalid) {
                _scale.selecting = true;
                _scale.entityId = entity;
                _scale.handle = ScaleHandle::None;
                Editor::UI::Providers::useSelection().selectEntity(_reg->entity_from_index(entity));
            } else {
                _scale.selecting = false;
                _scale.dragging = false;
                _scale.entityId = invalid;
                _scale.handle = ScaleHandle::None;
                Editor::UI::Providers::useSelection().clearSelection();
            }
            return;
        }

        if (!_scale.selecting || _scale.entityId == invalid)
            return;

        if (!_scale.dragging && ctx.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ScaleHandle hoveredHandle = getHoveredScaleHandle(ctx, _scale.entityId, io.MousePos);

            if (hoveredHandle != ScaleHandle::None) {
                _scale.dragging = true;
                _scale.handle = hoveredHandle;
                _scale.startMouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);

                auto& p = _positions->get(_scale.entityId);
                auto& b = _bounds->get(_scale.entityId);

                _scale.startX = p._x;
                _scale.startY = p._y;
                _scale.startW = b._width;
                _scale.startH = b._height;
            }
        }

        if (_scale.dragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            _scale.dragging = false;
            _scale.handle = ScaleHandle::None;
            return;
        }

        if (!_scale.dragging)
            return;

        if (_scale.entityId >= _positions->size() || !_positions->has(_scale.entityId) ||
            _scale.entityId >= _bounds->size() || !_bounds->has(_scale.entityId)) {
            _scale.dragging = false;
            _scale.selecting = false;
            _scale.entityId = invalid;
            _scale.handle = ScaleHandle::None;
            return;
        }

        ImVec2 mouseWorld = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
        ImVec2 delta(mouseWorld.x - _scale.startMouseWorld.x,
                    mouseWorld.y - _scale.startMouseWorld.y);

        auto& p = _positions->get(_scale.entityId);
        auto& b = _bounds->get(_scale.entityId);

        float newX = _scale.startX;
        float newY = _scale.startY;
        float newW = _scale.startW;
        float newH = _scale.startH;

        const bool preserveRatio = io.KeyShift;
        const float aspectRatio = _scale.startW / _scale.startH;

        switch (_scale.handle) {
            case ScaleHandle::BottomRight:
                newW = _scale.startW + delta.x;
                newH = _scale.startH + delta.y;
                if (preserveRatio) {
                    if (std::abs(delta.x) > std::abs(delta.y))
                        newH = newW / aspectRatio;
                    else
                        newW = newH * aspectRatio;
                }
                break;

            case ScaleHandle::BottomLeft:
                newX = _scale.startX + delta.x;
                newW = _scale.startW - delta.x;
                newH = _scale.startH + delta.y;
                if (preserveRatio) {
                    if (std::abs(delta.x) > std::abs(delta.y)) {
                        newH = newW / aspectRatio;
                    } else {
                        newW = newH * aspectRatio;
                        newX = _scale.startX + _scale.startW - newW;
                    }
                }
                break;

            case ScaleHandle::TopRight:
                newY = _scale.startY + delta.y;
                newW = _scale.startW + delta.x;
                newH = _scale.startH - delta.y;
                if (preserveRatio) {
                    if (std::abs(delta.x) > std::abs(delta.y)) {
                        newH = newW / aspectRatio;
                        newY = _scale.startY + _scale.startH - newH;
                    } else {
                        newW = newH * aspectRatio;
                    }
                }
                break;

            case ScaleHandle::TopLeft:
                newX = _scale.startX + delta.x;
                newY = _scale.startY + delta.y;
                newW = _scale.startW - delta.x;
                newH = _scale.startH - delta.y;
                if (preserveRatio) {
                    if (std::abs(delta.x) > std::abs(delta.y)) {
                        newH = newW / aspectRatio;
                        newY = _scale.startY + _scale.startH - newH;
                    } else {
                        newW = newH * aspectRatio;
                        newX = _scale.startX + _scale.startW - newW;
                    }
                }
                break;

            case ScaleHandle::None:
                return;
        }

        const float minSize = 10.f;
        if (newW < minSize) newW = minSize;
        if (newH < minSize) newH = minSize;

        p._x = newX;
        p._y = newY;
        b._width = newW;
        b._height = newH;
    }

    std::size_t Viewport::findEntityUnderMouse(const ImVec2& mouseWorld) const
    {
        if (!_positions || !_drawables || !_bounds)
            return static_cast<std::size_t>(-1);

        std::size_t found = static_cast<std::size_t>(-1);

        for (auto [d, p, b, entity] : zipper(*_drawables, *_positions, *_bounds)) {
            const float left = p._x;
            const float top = p._y;
            const float right = p._x + b._width;
            const float bottom = p._y + b._height;

            const bool inside =
                mouseWorld.x >= left && mouseWorld.x <= right &&
                mouseWorld.y >= top  && mouseWorld.y <= bottom;

            if (inside)
                found = static_cast<std::size_t>(entity);
        }
        return found;
    }

    void Viewport::handleCameraPan(const ViewportContext& ctx, const ImGuiIO& io) { 
        if (!_drag.active && ctx.hovered && io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) { 
            _panning = true; 
            _panStartMouse = io.MousePos;
            _panStartCamera = _camera;
        } 

        if (!_panning) 
            return;
        
        if (!io.KeyCtrl || !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            _panning = false; 
            return;
        }
        ImVec2 mouse = io.MousePos; ImVec2 drag(mouse.x - _panStartMouse.x, mouse.y - _panStartMouse.y);
        _camera.x = _panStartCamera.x - drag.x / _zoom;
        _camera.y = _panStartCamera.y - drag.y / _zoom;
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    void Viewport::drawBackground(const ViewportContext& ctx) const
    {
        ctx.drawList->AddRectFilled(ctx.origin, ctx.end, IM_COL32(25, 25, 30, 255));
    }

    void Viewport::drawScreenBounds(const ViewportContext& ctx) const
    {
        auto& pctx = Editor::ProjectContext::getInstance();
        ImVec2 p0 = worldToScreen(ctx, 0.f, 0.f);
        ImVec2 p1 = worldToScreen(ctx, pctx.screenWidth(), pctx.screenHeight());
        ctx.drawList->AddRectFilled(p0, p1, IM_COL32(15, 15, 20, 255));
        ctx.drawList->AddRect(p0, p1, IM_COL32(80, 80, 120, 255), 0.f, 0, 2.f);

        // Draw resize handle at bottom-right corner
        float hs = 8.f;
        ImVec2 hMin = {p1.x - hs, p1.y - hs};
        ImVec2 hMax = {p1.x + hs, p1.y + hs};
        ctx.drawList->AddRectFilled(hMin, hMax, IM_COL32(120, 120, 180, 255));
    }

    void Viewport::drawGrid(const ViewportContext& ctx) const
    {
        float cellSize = 50.f;

        // Adapt cell size to zoom so grid doesn't get too dense or too sparse
        float screenCell = cellSize * _zoom;
        while (screenCell < 30.f) { screenCell *= 2.f; cellSize *= 2.f; }
        while (screenCell > 120.f) { screenCell *= 0.5f; cellSize *= 0.5f; }

        // World-space range visible in the viewport
        ImVec2 topLeft = screenToWorld(ctx, ctx.origin.x, ctx.origin.y);
        ImVec2 botRight = screenToWorld(ctx, ctx.end.x, ctx.end.y);

        float startX = std::floor(topLeft.x / cellSize) * cellSize;
        float startY = std::floor(topLeft.y / cellSize) * cellSize;

        ImU32 color = IM_COL32(255, 255, 255, 18);

        for (float x = startX; x <= botRight.x; x += cellSize) {
            ImVec2 p0 = worldToScreen(ctx, x, topLeft.y);
            ImVec2 p1 = worldToScreen(ctx, x, botRight.y);
            ctx.drawList->AddLine(p0, p1, color);
        }
        for (float y = startY; y <= botRight.y; y += cellSize) {
            ImVec2 p0 = worldToScreen(ctx, topLeft.x, y);
            ImVec2 p1 = worldToScreen(ctx, botRight.x, y);
            ctx.drawList->AddLine(p0, p1, color);
        }
    }

    void Viewport::handleScreenResize(const ViewportContext& ctx, const ImGuiIO& io)
    {
        auto& pctx = Editor::ProjectContext::getInstance();
        ImVec2 corner = worldToScreen(ctx, pctx.screenWidth(), pctx.screenHeight());
        float hs = 8.f;

        bool overHandle = ctx.hovered &&
            io.MousePos.x >= corner.x - hs && io.MousePos.x <= corner.x + hs &&
            io.MousePos.y >= corner.y - hs && io.MousePos.y <= corner.y + hs;

        if (overHandle)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);

        if (overHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            _screenResizing = true;
            _screenResizeStart = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
            _screenStartW = pctx.screenWidth();
            _screenStartH = pctx.screenHeight();
        }

        if (_screenResizing) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                ImVec2 cur = screenToWorld(ctx, io.MousePos.x, io.MousePos.y);
                float newW = _screenStartW + (cur.x - _screenResizeStart.x);
                float newH = _screenStartH + (cur.y - _screenResizeStart.y);
                if (newW < 100.f) newW = 100.f;
                if (newH < 100.f) newH = 100.f;
                pctx.setScreenSize(newW, newH);
            } else {
                _screenResizing = false;
            }
        }
    }

    void Viewport::drawEntities(const ViewportContext& ctx) const
    {
        if (!_positions || !_drawables || !_bounds) return;

        auto& sel = Editor::UI::Providers::useSelection();
        
        for (auto [d, p, b, entity] : zipper(*_drawables, *_positions, *_bounds)) {
            ImVec2 p0 = worldToScreen(ctx, p._x, p._y);
            ImVec2 p1 = ImVec2(p0.x + b._width * _zoom, p0.y + b._height * _zoom);

            ImU32 color = IM_COL32(b._color.r, b._color.g, b._color.b, b._color.a);
            ctx.drawList->AddRectFilled(p0, p1, color);

            if (sel.hasSelection() && sel.getSelectedEntity() == ECS::Entity::entity(entity)) {
                ctx.drawList->AddRect(p0, p1, IM_COL32(50, 150, 255, 255), 0.f, 0, 2.f);
            }
        }
    }
    void Viewport::update(ECS::registry &reg)
    {
        _reg = &reg;
    }
}
