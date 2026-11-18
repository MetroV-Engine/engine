// Interactive test bench for the collision geometry (src/Engine/Common/Physic/Collision):
// move shapes with the mouse and see what collide(), raycast() and sweep() return.
//
// Modes (keys 1, 2, 3, or the panel):
//   Collide  the selected shape against every other one: contact point, normal,
//            depth, and where the shape goes once pushed out.
//   Raycast  a ray between two handles: every hit, its normal, the first one.
//   Sweep    the selected shape moved by the arrow: where it stops, the normal,
//            and what a check at the end of the move alone would miss.
//
// Mouse: left drag moves a shape or a handle (on empty space, it pans, as the
// right and middle buttons do); the wheel zooms. Keys: Delete removes the selected shape, G toggles the 0.25 m grid
// snap (exact touches and ties, as with tiles).
//
// "World offset" runs every query with the scene moved far from the origin, as
// positions stored there would be, and shows how far the results move.
//
// Usage: demo_collision [--screenshot file.bmp] [--mode collide|raycast|sweep] [--offset metres]
//        --screenshot renders a few frames, saves the image and quits.

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "Engine/Common/Physic/Collision/Collision.hpp"

using namespace Engine::Collision;

namespace {
    constexpr float SnapStep = 0.25f;
    constexpr float HandleRadius = 7.f;  // pixels
    constexpr float PanelWidth = 380.f;

    enum class Mode { Collide = 0, Raycast = 1, Sweep = 2 };

    // ------------------------------------------------------------------ scene

    struct Shape {
        enum class Kind { Rectangle, Circle };
        Kind kind = Kind::Rectangle;
        Vector2f center;
        Vector2f halfSize{0.5f, 0.5f};  // rectangles
        float radius = 0.5f;            // circles
    };

    // The frame the queries run in: the scene moved by `offset` metres on both axes and
    // rounded to float, as positions stored that far from the origin would be. Results
    // come back to the scene's frame for drawing.
    struct Frame {
        double offset = 0.0;

        [[nodiscard]] Vector2f in(const Vector2f& p) const {
            return {static_cast<float>(double(p.x) + offset), static_cast<float>(double(p.y) + offset)};
        }
        [[nodiscard]] Vector2f out(const Vector2f& p) const {
            return {static_cast<float>(double(p.x) - offset), static_cast<float>(double(p.y) - offset)};
        }
        [[nodiscard]] Rectangle rectangle(const Shape& s) const {
            return {in(s.center - s.halfSize), in(s.center + s.halfSize)};
        }
        [[nodiscard]] Circle circle(const Shape& s) const { return {in(s.center), s.radius}; }

        // Calls f with the shape as a Rectangle or a Circle.
        template <typename F>
        auto with(const Shape& s, F&& f) const {
            return s.kind == Shape::Kind::Rectangle ? f(rectangle(s)) : f(circle(s));
        }
    };

    std::optional<Contact> collideShapes(const Shape& a, const Shape& b, const Frame& frame) {
        std::optional<Contact> contact = frame.with(a, [&](const auto& first) {
            return frame.with(b, [&](const auto& second) { return collide(first, second); });
        });
        if (contact) {
            contact->point = frame.out(contact->point);
        }
        return contact;
    }

    bool overlapShapes(const Shape& a, const Shape& b, const Frame& frame) {
        return frame.with(a, [&](const auto& first) {
            return frame.with(b, [&](const auto& second) { return overlaps(first, second); });
        });
    }

    std::optional<Hit> raycastShape(const Vector2f& origin, const Vector2f& delta, const Shape& s, const Frame& frame) {
        std::optional<Hit> hit = frame.with(s, [&](const auto& shape) { return raycast(frame.in(origin), delta, shape); });
        if (hit) {
            hit->point = frame.out(hit->point);
        }
        return hit;
    }

    std::optional<Hit> sweepShape(const Shape& moving, const Vector2f& delta, const Shape& target, const Frame& frame) {
        std::optional<Hit> hit = frame.with(moving, [&](const auto& m) {
            return frame.with(target, [&](const auto& t) { return sweep(m, delta, t); });
        });
        if (hit) {
            hit->point = frame.out(hit->point);
        }
        return hit;
    }

    bool containsPoint(const Shape& s, const Vector2f& p) {
        const Frame local{};
        return local.with(s, [&](const auto& shape) { return contains(shape, p); });
    }

    std::vector<Shape> defaultScene() {
        const auto rect = [](float x, float y, float hx, float hy) { return Shape{Shape::Kind::Rectangle, {x, y}, {hx, hy}, 0.5f}; };
        const auto circle = [](float x, float y, float r) { return Shape{Shape::Kind::Circle, {x, y}, {r, r}, r}; };
        return {
            rect(-6.f, 1.f, 0.4f, 0.75f),   // 0: the player, selected first
            rect(0.f, -0.5f, 9.f, 0.5f),    // floor
            rect(-9.5f, 3.5f, 0.5f, 4.5f),  // left wall
            rect(4.f, 2.25f, 1.5f, 0.25f),  // platform
            rect(-2.f, 1.5f, 0.05f, 1.5f),  // thin wall: a check at the end of a fast move misses it
            circle(1.5f, 4.f, 0.5f),        // a ball
            circle(-4.f, 3.5f, 0.75f),      // a round obstacle
        };
    }

    // ------------------------------------------------------------------ view

    struct View {
        Vector2f center{-1.f, 2.5f};  // world point at the middle of the scene area
        float scale = 46.f;           // pixels per metre
        ImVec2 middle;                // screen point of `center`

        [[nodiscard]] ImVec2 toScreen(const Vector2f& w) const {
            return {middle.x + (w.x - center.x) * scale, middle.y - (w.y - center.y) * scale};
        }
        [[nodiscard]] Vector2f toWorld(const ImVec2& s) const {
            return {center.x + (s.x - middle.x) / scale, center.y - (s.y - middle.y) / scale};
        }
    };

    // ------------------------------------------------------------------ drawing

    namespace Color {
        const ImU32 Background = IM_COL32(22, 24, 29, 255);
        const ImU32 GridMinor = IM_COL32(255, 255, 255, 9);
        const ImU32 GridMajor = IM_COL32(255, 255, 255, 22);
        const ImU32 Axis = IM_COL32(255, 255, 255, 50);
        const ImU32 Fill = IM_COL32(78, 96, 124, 210);
        const ImU32 Outline = IM_COL32(150, 172, 204, 255);
        const ImU32 Selected = IM_COL32(255, 170, 60, 255);
        const ImU32 Ghost = IM_COL32(255, 170, 60, 120);
        const ImU32 Overlap = IM_COL32(196, 72, 72, 220);
        const ImU32 Touch = IM_COL32(200, 170, 60, 220);
        const ImU32 Normal = IM_COL32(96, 222, 128, 255);
        const ImU32 Ray = IM_COL32(250, 222, 96, 255);
        const ImU32 RayFaint = IM_COL32(250, 222, 96, 70);
        const ImU32 HitPoint = IM_COL32(255, 112, 200, 255);
        const ImU32 Text = IM_COL32(230, 232, 236, 255);
        const ImU32 TextMuted = IM_COL32(160, 164, 172, 255);
        const ImU32 White = IM_COL32(255, 255, 255, 255);
    }

    void drawShape(ImDrawList* dl, const View& view, const Shape& s, ImU32 fill, ImU32 outline, float thickness) {
        if (s.kind == Shape::Kind::Rectangle) {
            const ImVec2 a = view.toScreen({s.center.x - s.halfSize.x, s.center.y + s.halfSize.y});
            const ImVec2 b = view.toScreen({s.center.x + s.halfSize.x, s.center.y - s.halfSize.y});
            if (fill != 0) {
                dl->AddRectFilled(a, b, fill);
            }
            dl->AddRect(a, b, outline, 0.f, 0, thickness);
        } else {
            const ImVec2 c = view.toScreen(s.center);
            if (fill != 0) {
                dl->AddCircleFilled(c, s.radius * view.scale, fill, 64);
            }
            dl->AddCircle(c, s.radius * view.scale, outline, 64, thickness);
        }
    }

    void drawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color, float thickness = 2.f) {
        dl->AddLine(from, to, color, thickness);
        const float dx = to.x - from.x, dy = to.y - from.y;
        const float length = std::sqrt(dx * dx + dy * dy);
        if (length < 1.f) {
            return;
        }
        const float ux = dx / length, uy = dy / length, head = std::min(10.f, length * 0.5f);
        dl->AddTriangleFilled(to, {to.x - ux * head - uy * head * 0.5f, to.y - uy * head + ux * head * 0.5f},
                              {to.x - ux * head + uy * head * 0.5f, to.y - uy * head - ux * head * 0.5f}, color);
    }

    void drawGrid(ImDrawList* dl, const View& view, ImVec2 size) {
        const Vector2f topLeft = view.toWorld({0.f, 0.f});
        const Vector2f bottomRight = view.toWorld(size);
        const float step = view.scale > 70.f ? SnapStep : 1.f;
        for (float x = std::floor(topLeft.x / step) * step; x <= bottomRight.x; x += step) {
            const bool major = std::fmod(std::fabs(x), 1.f) < 1e-4f;
            const float sx = view.toScreen({x, 0.f}).x;
            dl->AddLine({sx, 0.f}, {sx, size.y}, std::fabs(x) < 1e-4f ? Color::Axis : major ? Color::GridMajor : Color::GridMinor);
        }
        for (float y = std::floor(bottomRight.y / step) * step; y <= topLeft.y; y += step) {
            const bool major = std::fmod(std::fabs(y), 1.f) < 1e-4f;
            const float sy = view.toScreen({0.f, y}).y;
            dl->AddLine({0.f, sy}, {size.x, sy}, std::fabs(y) < 1e-4f ? Color::Axis : major ? Color::GridMajor : Color::GridMinor);
        }
    }

    void label(ImDrawList* dl, ImVec2 at, ImU32 color, const char* text) { dl->AddText({at.x + 6.f, at.y - 16.f}, color, text); }

    std::string vec(const Vector2f& v) {
        char buffer[64];
        std::snprintf(buffer, sizeof buffer, "(%.3f, %.3f)", double(v.x), double(v.y));
        return buffer;
    }

    // ------------------------------------------------------------------ the demo

    struct Demo {
        std::vector<Shape> shapes = defaultScene();
        int selected = 0;
        Mode mode = Mode::Collide;
        bool snap = false;
        int offsetIndex = 0;
        Vector2f rayFrom{-8.f, 5.f};
        Vector2f rayTo{7.f, 1.5f};
        Vector2f sweepDelta{9.f, 0.f};
        View view;

        // dragging
        enum class Drag { None, Shape, RayFrom, RayTo, SweepEnd, Pan } drag = Drag::None;
        Vector2f grabOffset;

        static constexpr double Offsets[] = {0.0, 1'000.0, 10'000.0, 100'000.0};
        [[nodiscard]] Frame frame() const { return {Offsets[offsetIndex]}; }
        [[nodiscard]] bool hasSelection() const { return selected >= 0 && selected < static_cast<int>(shapes.size()); }

        [[nodiscard]] Vector2f snapped(const Vector2f& p) const {
            return snap ? Vector2f(std::round(p.x / SnapStep) * SnapStep, std::round(p.y / SnapStep) * SnapStep) : p;
        }

        void handleInput(ImVec2 sceneSize) {
            const ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard) {
                if (ImGui::IsKeyPressed(ImGuiKey_1)) mode = Mode::Collide;
                if (ImGui::IsKeyPressed(ImGuiKey_2)) mode = Mode::Raycast;
                if (ImGui::IsKeyPressed(ImGuiKey_3)) mode = Mode::Sweep;
                if (ImGui::IsKeyPressed(ImGuiKey_G)) snap = !snap;
                if (ImGui::IsKeyPressed(ImGuiKey_Delete) && hasSelection()) {
                    shapes.erase(shapes.begin() + selected);
                    selected = -1;
                }
            }
            if (io.WantCaptureMouse && drag == Drag::None) {
                return;
            }
            const ImVec2 mouse = io.MousePos;
            const Vector2f world = view.toWorld(mouse);
            const auto near = [&](const Vector2f& handle) {
                const ImVec2 h = view.toScreen(handle);
                return (h.x - mouse.x) * (h.x - mouse.x) + (h.y - mouse.y) * (h.y - mouse.y) <= (HandleRadius + 3.f) * (HandleRadius + 3.f);
            };

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouse.x < sceneSize.x) {
                if (mode == Mode::Raycast && near(rayTo)) {
                    drag = Drag::RayTo;
                } else if (mode == Mode::Raycast && near(rayFrom)) {
                    drag = Drag::RayFrom;
                } else if (mode == Mode::Sweep && hasSelection() && near(shapes[std::size_t(selected)].center + sweepDelta)) {
                    drag = Drag::SweepEnd;
                } else {
                    drag = Drag::Pan;  // empty space: pan, and keep the selection
                    for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {  // topmost first
                        if (containsPoint(shapes[std::size_t(i)], world)) {
                            selected = i;
                            drag = Drag::Shape;
                            grabOffset = shapes[std::size_t(i)].center - world;
                            break;
                        }
                    }
                }
            }
            if ((ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) && mouse.x < sceneSize.x) {
                drag = Drag::Pan;
            }

            switch (drag) {
                case Drag::Shape:
                    if (hasSelection()) shapes[std::size_t(selected)].center = snapped(world + grabOffset);
                    break;
                case Drag::RayFrom: rayFrom = snapped(world); break;
                case Drag::RayTo: rayTo = snapped(world); break;
                case Drag::SweepEnd:
                    if (hasSelection()) sweepDelta = snapped(world) - shapes[std::size_t(selected)].center;
                    break;
                case Drag::Pan: view.center -= Vector2f(io.MouseDelta.x / view.scale, -io.MouseDelta.y / view.scale); break;
                case Drag::None: break;
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
                !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                drag = Drag::None;
            }

            if (io.MouseWheel != 0.f && !io.WantCaptureMouse) {  // zoom around the cursor
                const Vector2f before = view.toWorld(mouse);
                view.scale = std::clamp(view.scale * std::pow(1.15f, io.MouseWheel), 8.f, 600.f);
                const Vector2f after = view.toWorld(mouse);
                view.center += before - after;
            }
        }

        // ---------------------------------------------------------------- per mode

        void collideMode(ImDrawList* dl) {
            if (!hasSelection()) {
                return;
            }
            const Shape& me = shapes[std::size_t(selected)];
            ImGui::SeparatorText("Contacts of the selected shape");
            ImGui::TextDisabled("normal: from the other shape towards the selected one");
            // Shapes first, the selected one on top, then the contacts over them.
            std::vector<std::pair<std::size_t, Contact>> contacts;
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                if (static_cast<int>(i) == selected) continue;
                const std::optional<Contact> contact = collideShapes(shapes[i], me, frame());
                drawShape(dl, view, shapes[i], contact ? (contact->depth > 0.f ? Color::Overlap : Color::Touch) : Color::Fill, Color::Outline, 1.f);
                if (contact) contacts.emplace_back(i, *contact);
            }
            drawShape(dl, view, me, Color::Fill, Color::Selected, 2.5f);

            const Contact* deepest = nullptr;
            for (const auto& [i, contact] : contacts) {
                // where the selected shape goes once pushed out of this one
                Shape pushed = me;
                pushed.center += contact.normal * contact.depth;
                drawShape(dl, view, pushed, 0, Color::Ghost, 1.5f);
                const ImVec2 p = view.toScreen(contact.point);
                drawArrow(dl, p, view.toScreen(contact.point + contact.normal * std::max(0.6f, contact.depth)), Color::Normal);
                dl->AddCircleFilled(p, 4.f, Color::White);
                char text[64];
                std::snprintf(text, sizeof text, "#%zu depth %.3f", i, double(contact.depth));
                label(dl, p, Color::Text, text);
                ImGui::Text("#%zu  depth %.4f m", i, double(contact.depth));
                ImGui::TextDisabled("     normal %s  point %s", vec(contact.normal).c_str(), vec(contact.point).c_str());
                if (!deepest || contact.depth > deepest->depth) deepest = &contact;
            }
            if (!deepest) {
                ImGui::TextDisabled("no overlap");
            } else if (ImGui::Button("Push out of the deepest contact")) {
                shapes[std::size_t(selected)].center += deepest->normal * deepest->depth;
            }
        }

        void raycastMode(ImDrawList* dl) {
            for (const Shape& s : shapes) drawShape(dl, view, s, Color::Fill, Color::Outline, 1.f);
            const Vector2f delta = rayTo - rayFrom;
            ImGui::SeparatorText("Hits of the ray");
            ImGui::TextDisabled("a ray starting inside a shape ignores it");
            std::optional<Hit> first;
            std::size_t firstIndex = 0;
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                const std::optional<Hit> hit = raycastShape(rayFrom, delta, shapes[i], frame());
                if (!hit) continue;
                if (!first || hit->fraction < first->fraction) {
                    first = hit;
                    firstIndex = i;
                }
                const ImVec2 p = view.toScreen(hit->point);
                drawArrow(dl, p, view.toScreen(hit->point + hit->normal * 0.5f), Color::Normal, 1.5f);
                dl->AddCircleFilled(p, 3.5f, Color::HitPoint);
                ImGui::Text("#%zu  fraction %.4f", i, double(hit->fraction));
                ImGui::TextDisabled("     point %s  normal %s", vec(hit->point).c_str(), vec(hit->normal).c_str());
            }
            const ImVec2 a = view.toScreen(rayFrom), b = view.toScreen(rayTo);
            if (first) {
                const ImVec2 h = view.toScreen(first->point);
                dl->AddLine(a, h, Color::Ray, 2.f);
                dl->AddLine(h, b, Color::RayFaint, 1.5f);
                dl->AddCircle(h, 7.f, Color::HitPoint, 24, 2.f);
                char text[64];
                std::snprintf(text, sizeof text, "first hit #%zu, fraction %.3f", firstIndex, double(first->fraction));
                label(dl, h, Color::Text, text);
            } else {
                dl->AddLine(a, b, Color::Ray, 2.f);
                ImGui::TextDisabled("no hit");
            }
            dl->AddCircleFilled(a, HandleRadius, Color::Ray);
            dl->AddCircle(b, HandleRadius, Color::Ray, 24, 2.f);
            precisionReadout("first hit point", first ? std::optional<Vector2f>(first->point) : std::nullopt, [&](const Frame& f) {
                std::optional<Vector2f> best;
                float bestFraction = 2.f;
                for (const Shape& s : shapes) {
                    const std::optional<Hit> hit = raycastShape(rayFrom, delta, s, f);
                    if (hit && hit->fraction < bestFraction) {
                        bestFraction = hit->fraction;
                        best = hit->point;
                    }
                }
                return best;
            });
        }

        void sweepMode(ImDrawList* dl) {
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                if (static_cast<int>(i) != selected) drawShape(dl, view, shapes[i], Color::Fill, Color::Outline, 1.f);
            }
            if (!hasSelection()) {
                return;
            }
            const Shape& me = shapes[std::size_t(selected)];
            ImGui::SeparatorText("Sweep of the selected shape");
            ImGui::TextDisabled("drag the arrow's head to change the move");
            std::optional<Hit> first;
            std::size_t firstIndex = 0;
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                if (static_cast<int>(i) == selected) continue;
                const std::optional<Hit> hit = sweepShape(me, sweepDelta, shapes[i], frame());
                if (hit && (!first || hit->fraction < first->fraction)) {
                    first = hit;
                    firstIndex = i;
                }
            }

            // the end of the move, tested alone: what a discrete check would see
            Shape atEnd = me;
            atEnd.center += sweepDelta;
            bool endOverlaps = false;
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                if (static_cast<int>(i) != selected && overlapShapes(atEnd, shapes[i], frame())) endOverlaps = true;
            }
            drawShape(dl, view, atEnd, 0, endOverlaps ? Color::Overlap : Color::TextMuted, 1.f);
            label(dl, view.toScreen(atEnd.center), Color::TextMuted, "end of the move");

            const ImVec2 start = view.toScreen(me.center), end = view.toScreen(me.center + sweepDelta);
            drawShape(dl, view, me, Color::Fill, Color::Selected, 2.5f);
            if (first) {
                Shape atImpact = me;
                atImpact.center = first->point;
                drawArrow(dl, start, view.toScreen(first->point), Color::Ray, 2.f);
                dl->AddLine(view.toScreen(first->point), end, Color::RayFaint, 1.5f);
                drawShape(dl, view, atImpact, IM_COL32(255, 170, 60, 60), Color::Selected, 2.f);
                drawArrow(dl, view.toScreen(first->point), view.toScreen(first->point + first->normal * 0.8f), Color::Normal);
                char text[64];
                std::snprintf(text, sizeof text, "impact, fraction %.3f", double(first->fraction));
                label(dl, view.toScreen(first->point), Color::Text, text);

                ImGui::Text("first contact: #%zu at fraction %.4f", firstIndex, double(first->fraction));
                ImGui::TextDisabled("center at impact %s", vec(first->point).c_str());
                ImGui::TextDisabled("normal %s", vec(first->normal).c_str());
                if (!overlapShapes(atEnd, shapes[firstIndex], frame())) {
                    ImGui::TextColored(ImVec4(1.f, 0.67f, 0.24f, 1.f), "Tested at the end only, the move would go");
                    ImGui::TextColored(ImVec4(1.f, 0.67f, 0.24f, 1.f), "through #%zu: the sweep catches it.", firstIndex);
                }
            } else {
                drawArrow(dl, start, end, Color::Ray, 2.f);
                ImGui::TextDisabled("no contact along the move");
            }
            dl->AddCircle(end, HandleRadius, Color::Ray, 24, 2.f);
            ImGui::TextDisabled("end of the move alone: %s", endOverlaps ? "overlaps" : "no overlap");
            precisionReadout("center at impact", first ? std::optional<Vector2f>(first->point) : std::nullopt, [&](const Frame& f) {
                std::optional<Vector2f> best;
                float bestFraction = 2.f;
                for (std::size_t i = 0; i < shapes.size(); ++i) {
                    if (static_cast<int>(i) == selected) continue;
                    const std::optional<Hit> hit = sweepShape(me, sweepDelta, shapes[i], f);
                    if (hit && hit->fraction < bestFraction) {
                        bestFraction = hit->fraction;
                        best = hit->point;
                    }
                }
                return best;
            });
        }

        // How far the result moves when the same query runs `offset` metres from the origin.
        template <typename Query>
        void precisionReadout(const char* what, const std::optional<Vector2f>& here, Query query) {
            if (offsetIndex == 0 || !here) {
                return;
            }
            const std::optional<Vector2f> atOrigin = query(Frame{0.0});
            if (!atOrigin) {
                ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "at the origin, the same query finds no hit");
                return;
            }
            const Vector2f d = *here - *atOrigin;
            const double step = std::ldexp(1.0, std::ilogb(Offsets[offsetIndex]) - 23);
            ImGui::Text("%s moved by %.1f um vs the same query at the origin.", what, std::sqrt(double(d.x) * d.x + double(d.y) * d.y) * 1e6);
            ImGui::TextDisabled("Most of it comes from the positions themselves: stored as floats there, they are "
                                "rounded to the nearest %.1f um, before any collision code runs.", step * 1e6);
        }

        // ---------------------------------------------------------------- frame

        void frameUpdate() {
            const ImGuiIO& io = ImGui::GetIO();
            const ImVec2 sceneSize{io.DisplaySize.x - PanelWidth, io.DisplaySize.y};
            view.middle = {sceneSize.x * 0.5f, sceneSize.y * 0.55f};
            handleInput(sceneSize);

            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            dl->AddRectFilled({0.f, 0.f}, io.DisplaySize, Color::Background);
            drawGrid(dl, view, sceneSize);

            ImGui::SetNextWindowPos({io.DisplaySize.x - PanelWidth, 0.f});
            ImGui::SetNextWindowSize({PanelWidth, io.DisplaySize.y});
            ImGui::Begin("Collision demo", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::PushTextWrapPos(0.f);  // long lines wrap at the panel's edge

            int modeIndex = static_cast<int>(mode);
            ImGui::RadioButton("Collide (1)", &modeIndex, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Raycast (2)", &modeIndex, 1);
            ImGui::SameLine();
            ImGui::RadioButton("Sweep (3)", &modeIndex, 2);
            mode = static_cast<Mode>(modeIndex);

            ImGui::SeparatorText("Scene");
            if (ImGui::Button("+ Rectangle")) {
                shapes.push_back({Shape::Kind::Rectangle, snapped(view.center), {0.5f, 0.5f}, 0.5f});
                selected = static_cast<int>(shapes.size()) - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("+ Circle")) {
                shapes.push_back({Shape::Kind::Circle, snapped(view.center), {0.5f, 0.5f}, 0.5f});
                selected = static_cast<int>(shapes.size()) - 1;
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset")) {
                shapes = defaultScene();
                selected = 0;
            }
            ImGui::Checkbox("Snap to the 0.25 m grid (G)", &snap);
            ImGui::Combo("World offset", &offsetIndex, "0 m\0" "1 km\0" "10 km\0" "100 km\0");
            if (offsetIndex > 0) {
                const double step = std::ldexp(1.0, std::ilogb(Offsets[offsetIndex]) - 23);
                ImGui::TextDisabled("queries run at (%.0f, %.0f): a float step is %.1f um", Offsets[offsetIndex], Offsets[offsetIndex], step * 1e6);
            }

            if (hasSelection()) {
                Shape& s = shapes[std::size_t(selected)];
                ImGui::SeparatorText(s.kind == Shape::Kind::Rectangle ? "Selected: rectangle" : "Selected: circle");
                ImGui::DragFloat2("Center", &s.center.x, 0.01f, 0.f, 0.f, "%.3f");
                if (s.kind == Shape::Kind::Rectangle) {
                    ImGui::DragFloat2("Half size", &s.halfSize.x, 0.01f, 0.f, 50.f, "%.3f");
                } else {
                    ImGui::DragFloat("Radius", &s.radius, 0.01f, 0.f, 50.f, "%.3f");
                }
                if (ImGui::Button("Delete (Del)")) {
                    shapes.erase(shapes.begin() + selected);
                    selected = -1;
                }
            } else if (mode != Mode::Raycast) {
                ImGui::SeparatorText("Selected: none");
                ImGui::TextDisabled("click a shape to select it");
            }

            switch (mode) {
                case Mode::Collide:
                    if (!hasSelection()) for (const Shape& s : shapes) drawShape(dl, view, s, Color::Fill, Color::Outline, 1.f);
                    collideMode(dl);
                    break;
                case Mode::Raycast: raycastMode(dl); break;
                case Mode::Sweep: sweepMode(dl); break;
            }
            for (std::size_t i = 0; i < shapes.size(); ++i) {
                char text[8];
                std::snprintf(text, sizeof text, "#%zu", i);
                const ImVec2 c = view.toScreen(shapes[i].center);
                dl->AddText({c.x - 8.f, c.y - 7.f}, Color::TextMuted, text);
            }

            ImGui::SeparatorText("Controls");
            ImGui::TextDisabled("left drag: move a shape or a handle");
            ImGui::TextDisabled("drag empty space: pan   wheel: zoom");
            ImGui::TextDisabled("%.0f fps", double(io.Framerate));
            ImGui::PopTextWrapPos();
            ImGui::End();
        }
    };

    // ------------------------------------------------------------------ program

    struct Options {
        const char* screenshot = nullptr;
        Mode mode = Mode::Collide;
        int offsetIndex = 0;
    };

    Options parse(int argc, char** argv) {
        Options options;
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::strcmp(argv[i], "--screenshot") == 0) {
                options.screenshot = argv[++i];
            } else if (std::strcmp(argv[i], "--mode") == 0) {
                const char* m = argv[++i];
                options.mode = std::strcmp(m, "raycast") == 0 ? Mode::Raycast : std::strcmp(m, "sweep") == 0 ? Mode::Sweep : Mode::Collide;
            } else if (std::strcmp(argv[i], "--offset") == 0) {
                const double metres = std::atof(argv[++i]);
                options.offsetIndex = metres >= 1e5 ? 3 : metres >= 1e4 ? 2 : metres >= 1e3 ? 1 : 0;
            }
        }
        return options;
    }
}

int main(int argc, char** argv) {
    const Options options = parse(argc, argv);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("MetroV - collision demo", 1360, 820, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!renderer) {
        std::fprintf(stderr, "window or renderer creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;  // no imgui.ini next to the binary
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    Demo demo;
    demo.mode = options.mode;
    demo.offsetIndex = options.offsetIndex;
    int frames = 0;
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
                running = false;
            }
        }
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        demo.frameUpdate();
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        if (options.screenshot && ++frames == 3) {  // a few frames for the layout to settle
            if (SDL_Surface* surface = SDL_RenderReadPixels(renderer, nullptr)) {
                SDL_SaveBMP(surface, options.screenshot);
                SDL_DestroySurface(surface);
            }
            running = false;
        }
        SDL_RenderPresent(renderer);
    }

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
