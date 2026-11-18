// Visual check of FrictionSystem: boxes on surfaces, moved by the engine's own
// systems (GravitySystem, FrictionSystem, IntegrationSystem) at a fixed 60 Hz,
// next to what Coulomb's law predicts.
//
// Scenes (keys 1 to 4, or the panel):
//   Slide     three boxes launched at the same speed on ice, wood and rubber:
//             each stops at v0^2 / (2 muK g), marked on its lane, and never
//             moves back.
//   Slope     a box at rest on a slope: it stays while tan(angle) <= muS, and
//             slides at g (sin - muK cos) otherwise.
//   Conveyor  a box put on a moving belt: dragged at muK g up to the belt's
//             speed, then carried at it.
//   Push      hold Space to push a resting box: it stays while the push is at
//             most muS N, then accelerates at (F - muK N) / m. Release it and
//             it slides to a stop.
//
// The ground's support (what keeps a box from sinking into it) is not part of
// FrictionSystem: it will come from the collision response. Until then, the
// demo stands in for it (SupportSystem below): after each step, it puts the
// box back on its surface and removes its speed into it.
//
// Usage: demo_friction [--screenshot file.bmp] [--scene slide|slope|conveyor|push] [--seconds s] [--push newtons]
//        --screenshot runs the scene for s seconds (default 0), saves the image and quits.

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Surface.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/SurfaceContact.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Physics/Friction/FrictionSystem.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "Engine/System/Physics/Integration/IntegrationSystem.hpp"

using namespace Engine;
using V = Math::Vector2f;

namespace {
    constexpr double Dt = 1.0 / 60.0;
    constexpr float G = 9.81f;
    constexpr float PanelWidth = 400.f;
    constexpr int HistorySize = 360;  // 6 s of speed samples at 60 Hz

    enum class Scene { Slide = 0, Slope = 1, Conveyor = 2, Push = 3 };

    // ------------------------------------------------------------------ demo-only components

    /** A straight piece of ground: from `start`, along the unit `tangent`, for `length` metres. */
    struct Ground {
        V start;
        V tangent{1.f, 0.f};
        float length = 10.f;
        [[nodiscard]] V normal() const { return {-tangent.y, tangent.x}; }  // out of the ground, towards the box
        [[nodiscard]] V at(float along) const { return start + tangent * along; }
    };

    /** The size of a box: its center sits halfHeight above its ground. */
    struct Box {
        float halfWidth = 0.4f;
        float halfHeight = 0.4f;
    };

    /**
     * Stand-in for the collision response, which does not exist yet: keeps every
     * box on its ground. Runs after IntegrationSystem: integration has moved the
     * box a little into the ground (its weight is not held), so put it back and
     * remove its speed into the ground. Motion along the ground is left alone:
     * that part is FrictionSystem's, and what the demo checks.
     */
    class SupportSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                for (auto [entity, transform, body, contact, ground, box] :
                     world.view<Component::Transform, Component::RigidBody, Component::SurfaceContact, Ground, Box>()) {
                    const V n = contact.normal;
                    const float above = (transform.position - ground.start).dot(n) - box.halfHeight;
                    if (above < 0.f) {
                        transform.position -= n * above;
                    }
                    const float into = (body.velocity - contact.surfaceVelocity).dot(n);
                    if (into < 0.f) {
                        body.velocity -= n * into;
                    }
                }
            }

            const std::string name() const override { return "SupportSystem"; }
    };

    // ------------------------------------------------------------------ simulation

    /** A box the panel reports on: where it started, how its speed went. */
    struct Tracked {
        ECS::Entity entity;
        std::string label;
        ImU32 color = 0;
        float startAlong = 0.f;
        float speed0 = 0.f;
        std::vector<float> history;  // speed along the ground, one sample per step
        double stopTime = -1.0;      // when it came to rest, -1 while moving
        double slideStart = -1.0;    // when it started to slide (from rest)
        bool movedBack = false;
        float lastAlong = 0.f;
    };

    struct Simulation {
        ECS::Registry world;
        ECS::SystemManager systems;
        std::vector<Tracked> tracked;
        std::vector<Ground> grounds;  // for drawing
        double time = 0.0;
        int steps = 0;
        bool finished = false;  // a box reached the end of its ground

        Simulation() {
            world.setResource<Resource::GravitySettings>(Resource::GravitySettings{V(0.f, -G), Physic::GravitationalConstant<float>});
            systems.addSystem<System::GravitySystem>(world);
            systems.addSystem<System::FrictionSystem>(world);
            systems.addSystem<System::IntegrationSystem>(world);
            systems.addSystem<SupportSystem>(world);
        }

        Tracked& addBox(const Ground& ground, float along, float speed, Component::SurfaceContact contact, float mass,
                        const char* label, ImU32 color, Box box = {}) {
            contact.normal = ground.normal();
            const ECS::Entity e = world.spawnEntity(label);
            world.addComponent(e, Component::Transform{ground.at(along) + ground.normal() * box.halfHeight});
            Component::RigidBody body;
            body.mass = mass;
            body.velocity = ground.tangent * speed;
            world.addComponent(e, body);
            world.addComponent(e, contact);
            world.addComponent(e, ground);
            world.addComponent(e, box);
            grounds.push_back(ground);
            Tracked t;
            t.entity = e;
            t.label = label;
            t.color = color;
            t.startAlong = along;
            t.speed0 = speed;
            t.lastAlong = along;
            tracked.push_back(t);
            return tracked.back();
        }

        [[nodiscard]] float along(const Tracked& t) {
            const Ground& g = world.getComponent<Ground>(t.entity);
            return (world.getComponent<Component::Transform>(t.entity).position - g.start).dot(g.tangent);
        }
        [[nodiscard]] float speed(const Tracked& t) {
            return world.getComponent<Component::RigidBody>(t.entity).velocity.dot(world.getComponent<Ground>(t.entity).tangent);
        }

        void step() {
            if (finished) {
                return;
            }
            systems.update(world, Dt);
            time += Dt;
            ++steps;
            for (Tracked& t : tracked) {
                const float s = speed(t), a = along(t);
                const float relative = s - world.getComponent<Component::SurfaceContact>(t.entity).surfaceVelocity.dot(
                                               world.getComponent<Ground>(t.entity).tangent);
                if (t.history.size() == HistorySize) {
                    t.history.erase(t.history.begin());
                }
                t.history.push_back(s);
                // moving back: against the direction it was launched in (or, from rest, the one it took)
                const float direction = t.speed0 != 0.f ? t.speed0 : (t.slideStart >= 0.0 ? 1.f : 0.f);
                if (direction != 0.f && (a - t.lastAlong) * direction < -1e-6f) {
                    t.movedBack = true;
                }
                t.lastAlong = a;
                // At rest relative to its ground: stopped, or carried at the belt's speed.
                const bool resting = std::fabs(relative) <= 1e-5f;
                if (!resting && t.slideStart < 0.0 && t.speed0 == 0.f) {
                    t.slideStart = time - Dt;  // it was at rest at the start of this step
                }
                const bool wasMoving = t.speed0 != 0.f || t.slideStart >= 0.0;
                if (resting && wasMoving && t.stopTime < 0.0) {
                    t.stopTime = time;
                }
                const Ground& g = world.getComponent<Ground>(t.entity);
                if (a > g.length - 0.5f || a < 0.5f) {
                    finished = true;
                }
            }
        }
    };

    // ------------------------------------------------------------------ view and drawing

    struct View {
        V center;
        float scale = 30.f;  // pixels per metre
        ImVec2 middle;
        [[nodiscard]] ImVec2 toScreen(const V& w) const { return {middle.x + (w.x - center.x) * scale, middle.y - (w.y - center.y) * scale}; }
    };

    namespace Color {
        const ImU32 Background = IM_COL32(22, 24, 29, 255);
        const ImU32 Ground = IM_COL32(70, 66, 60, 255);
        const ImU32 GroundTop = IM_COL32(170, 160, 140, 255);
        const ImU32 Ice = IM_COL32(120, 190, 235, 255);
        const ImU32 Wood = IM_COL32(205, 150, 90, 255);
        const ImU32 Rubber = IM_COL32(230, 100, 110, 255);
        const ImU32 Crate = IM_COL32(235, 170, 70, 255);
        const ImU32 Marker = IM_COL32(120, 230, 140, 255);
        const ImU32 Push = IM_COL32(250, 222, 96, 255);
        const ImU32 Text = IM_COL32(230, 232, 236, 255);
        const ImU32 Belt = IM_COL32(110, 110, 120, 255);
    }

    void drawGround(ImDrawList* dl, const View& view, const Ground& g, ImU32 top, float beltOffset = -1.f) {
        const V n = g.normal(), a = g.start, b = g.at(g.length);
        dl->AddQuadFilled(view.toScreen(a), view.toScreen(b), view.toScreen(b - n * 0.6f), view.toScreen(a - n * 0.6f), Color::Ground);
        dl->AddLine(view.toScreen(a), view.toScreen(b), top, 3.f);
        if (beltOffset >= 0.f) {  // moving stripes on a belt
            for (float s = std::fmod(beltOffset, 1.f); s < g.length; s += 1.f) {
                dl->AddLine(view.toScreen(g.at(s) - n * 0.1f), view.toScreen(g.at(s) - n * 0.5f), Color::Belt, 2.f);
            }
        }
    }

    void drawBox(ImDrawList* dl, const View& view, Simulation& sim, const Tracked& t) {
        const V c = sim.world.getComponent<Component::Transform>(t.entity).position;
        const Ground& g = sim.world.getComponent<Ground>(t.entity);
        const Box& box = sim.world.getComponent<Box>(t.entity);
        const V u = g.tangent * box.halfWidth, n = g.normal() * box.halfHeight;
        dl->AddQuadFilled(view.toScreen(c - u - n), view.toScreen(c + u - n), view.toScreen(c + u + n), view.toScreen(c - u + n), t.color);
        dl->AddQuad(view.toScreen(c - u - n), view.toScreen(c + u - n), view.toScreen(c + u + n), view.toScreen(c - u + n), IM_COL32(0, 0, 0, 120), 1.5f);
        char text[48];
        std::snprintf(text, sizeof text, "%.2f m/s", double(sim.speed(t)));
        const ImVec2 p = view.toScreen(c + g.normal() * (box.halfHeight + 0.35f));
        dl->AddText({p.x - 22.f, p.y - 8.f}, Color::Text, text);
    }

    void drawMarker(ImDrawList* dl, const View& view, const Ground& g, float along, const char* text) {
        const V n = g.normal();
        dl->AddLine(view.toScreen(g.at(along) - n * 0.8f), view.toScreen(g.at(along) + n * 1.4f), Color::Marker, 2.f);
        const ImVec2 p = view.toScreen(g.at(along) - n * 0.9f);
        dl->AddText({p.x - 20.f, p.y + 2.f}, Color::Marker, text);
    }

    void drawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color, float thickness = 3.f) {
        dl->AddLine(from, to, color, thickness);
        const float dx = to.x - from.x, dy = to.y - from.y, length = std::sqrt(dx * dx + dy * dy);
        if (length < 1.f) return;
        const float ux = dx / length, uy = dy / length, head = 10.f;
        dl->AddTriangleFilled(to, {to.x - ux * head - uy * head * 0.5f, to.y - uy * head + ux * head * 0.5f},
                              {to.x - ux * head + uy * head * 0.5f, to.y - uy * head - ux * head * 0.5f}, color);
    }

    void plotSpeed(const Tracked& t, float low, float high) {
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImGui::ColorConvertU32ToFloat4(t.color));
        char overlay[64];
        std::snprintf(overlay, sizeof overlay, "%s: speed over the last 6 s", t.label.c_str());
        ImGui::PlotLines("##speed", t.history.data(), static_cast<int>(t.history.size()), 0, overlay, low, high, ImVec2(-1.f, 70.f));
        ImGui::PopStyleColor();
    }

    // ------------------------------------------------------------------ the demo

    struct Demo {
        Scene scene = Scene::Slide;
        std::unique_ptr<Simulation> sim;
        View view;
        bool paused = false;
        float timeScale = 1.f;
        double accumulator = 0.0;

        // parameters
        float slideSpeed = 6.f;
        float slopeDegrees = 30.f, slopeStatic = 0.5f, slopeKinetic = 0.3f;
        float beltSpeed = 3.f, beltStatic = 0.6f, beltKinetic = 0.3f;
        float pushForce = 40.f;
        bool pushing = false;
        static constexpr float PushMass = 10.f, PushStatic = 0.5f, PushKinetic = 0.3f;

        static Component::SurfaceContact contact(float muS, float muK, V surfaceVelocity = V::zero()) {
            Component::SurfaceContact c;
            c.staticFriction = muS;
            c.kineticFriction = muK;
            c.surfaceVelocity = surfaceVelocity;
            return c;
        }

        void reset() {
            sim = std::make_unique<Simulation>();
            accumulator = 0.0;
            switch (scene) {
                case Scene::Slide: {
                    const char* names[] = {"Ice (muK 0.1)", "Wood (muK 0.3)", "Rubber (muK 0.8)"};
                    const float muS[] = {0.15f, 0.5f, 1.f}, muK[] = {0.1f, 0.3f, 0.8f};
                    const ImU32 colors[] = {Color::Ice, Color::Wood, Color::Rubber};
                    for (int i = 0; i < 3; ++i) {
                        sim->addBox(Ground{V(0.f, -3.5f * float(i)), V(1.f, 0.f), 44.f}, 1.f, slideSpeed, contact(muS[i], muK[i]), 1.f, names[i], colors[i]);
                    }
                    view.center = V(22.f, -3.f);
                    break;
                }
                case Scene::Slope: {
                    const float a = slopeDegrees * 3.14159265f / 180.f;
                    const Ground slope{V(0.f, 0.f), V(std::cos(a), -std::sin(a)), 18.f};
                    sim->addBox(slope, 1.f, 0.f, contact(slopeStatic, slopeKinetic), 1.f, "Box", Color::Crate);
                    view.center = slope.at(9.f) + V(0.f, 1.f);
                    break;
                }
                case Scene::Conveyor:
                    sim->addBox(Ground{V(0.f, 0.f), V(1.f, 0.f), 40.f}, 1.f, 0.f, contact(beltStatic, beltKinetic, V(beltSpeed, 0.f)), 1.f, "Box", Color::Crate);
                    view.center = V(20.f, 1.f);
                    break;
                case Scene::Push:
                    sim->addBox(Ground{V(0.f, 0.f), V(1.f, 0.f), 40.f}, 2.f, 0.f, contact(PushStatic, PushKinetic), PushMass, "Crate (10 kg)",
                                Color::Wood, Box{0.6f, 0.6f});
                    view.center = V(20.f, 1.f);
                    break;
            }
        }

        void advance(double seconds) {
            accumulator += seconds;
            int budget = 8;  // at most 8 steps per frame, so a slow frame does not snowball
            while (accumulator >= Dt && budget-- > 0) {
                stepOnce();
                accumulator -= Dt;
            }
            if (budget < 0) accumulator = 0.0;
        }

        void stepOnce() {
            if (scene == Scene::Push && pushing && !sim->tracked.empty()) {
                // A push is a force like any other: added before the step's systems run.
                sim->world.getComponent<Component::RigidBody>(sim->tracked[0].entity).force += V(pushForce, 0.f);
            }
            sim->step();
        }

        // ---------------------------------------------------------------- per scene panels

        void slidePanel(ImDrawList* dl) {
            ImGui::SeparatorText("Slide");
            if (ImGui::SliderFloat("Launch speed (m/s)", &slideSpeed, 1.f, 8.f, "%.1f")) reset();
            ImGui::TextDisabled("Each box should stop on its green mark, at v0^2 / (2 muK g), and never move back.");
            for (Tracked& t : sim->tracked) {
                const Component::SurfaceContact& c = sim->world.getComponent<Component::SurfaceContact>(t.entity);
                const float a = c.kineticFriction * G;
                const float theory = t.speed0 * t.speed0 / (2.f * a);
                const float euler = theory - t.speed0 * float(Dt) / 2.f;  // what semi-implicit Euler covers at 60 Hz
                const float travelled = sim->along(t) - t.startAlong;
                drawMarker(dl, view, sim->world.getComponent<Ground>(t.entity), t.startAlong + theory, "theory");
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(t.color), "%s", t.label.c_str());
                ImGui::Text("  travelled %.3f m   theory %.3f m", double(travelled), double(theory));
                ImGui::TextDisabled("  expected at 60 Hz: %.3f m (semi-implicit Euler stops v0 dt / 2 early)", double(euler));
                if (t.stopTime >= 0.0) {
                    ImGui::Text("  stopped after %.3f s   theory %.3f s", t.stopTime, double(t.speed0 / a));
                } else {
                    ImGui::TextDisabled("  sliding...");
                }
                ImGui::Text("  moved back: %s", t.movedBack ? "YES (wrong)" : "never");
                plotSpeed(t, 0.f, slideSpeed);
            }
        }

        void slopePanel() {
            ImGui::SeparatorText("Slope");
            bool changed = ImGui::SliderFloat("Angle (degrees)", &slopeDegrees, 0.f, 60.f, "%.1f");
            changed |= ImGui::SliderFloat("muStatic", &slopeStatic, 0.f, 1.5f, "%.2f");
            changed |= ImGui::SliderFloat("muKinetic", &slopeKinetic, 0.f, 1.5f, "%.2f");
            slopeKinetic = std::min(slopeKinetic, slopeStatic);
            if (changed) reset();
            const float a = slopeDegrees * 3.14159265f / 180.f;
            const float critical = std::atan(slopeStatic) * 180.f / 3.14159265f;
            const bool holds = std::tan(a) <= slopeStatic;
            ImGui::Text("tan(angle) = %.3f, muStatic = %.3f", double(std::tan(a)), double(slopeStatic));
            ImGui::Text("Critical angle atan(muS) = %.1f degrees: %s", double(critical), holds ? "the box should HOLD" : "the box should SLIDE");
            Tracked& t = sim->tracked[0];
            const float theory = holds ? 0.f : G * (std::sin(a) - slopeKinetic * std::cos(a));
            const float measured = t.slideStart >= 0.0 && sim->time > t.slideStart ? sim->speed(t) / float(sim->time - t.slideStart) : 0.f;
            ImGui::Text("Acceleration along the slope: measured %.3f m/s^2", double(measured));
            ImGui::Text("                              theory   %.3f m/s^2", double(theory));
            ImGui::TextDisabled("theory: g (sin - muK cos) when it slides, 0 when it holds");
            ImGui::Text("Travelled %.3f m", double(sim->along(t) - t.startAlong));
            if (sim->finished) ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.55f, 1.f), "Reached the bottom after %.2f s (theory %.2f s)", sim->time,
                                                  double(std::sqrt(2.f * (sim->along(t) - t.startAlong) / std::max(theory, 1e-6f))));
            plotSpeed(t, 0.f, 12.f);
        }

        void conveyorPanel() {
            ImGui::SeparatorText("Conveyor belt");
            bool changed = ImGui::SliderFloat("Belt speed (m/s)", &beltSpeed, 0.f, 8.f, "%.1f");
            changed |= ImGui::SliderFloat("muKinetic", &beltKinetic, 0.05f, 1.f, "%.2f");
            if (changed) reset();
            Tracked& t = sim->tracked[0];
            const float theoryTime = beltSpeed / (beltKinetic * G);
            ImGui::Text("Box speed %.3f m/s, belt %.3f m/s", double(sim->speed(t)), double(beltSpeed));
            ImGui::Text("Dragged at muK g = %.2f m/s^2: belt speed after %.3f s (theory)", double(beltKinetic * G), double(theoryTime));
            if (t.stopTime >= 0.0) ImGui::Text("Reached the belt's speed after %.3f s, then carried by static friction", t.stopTime);
            ImGui::TextDisabled("at 60 Hz the step that finishes the catch-up ends up to 1/60 s after the theory");
            if (sim->finished) ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.55f, 1.f), "Reached the end of the belt");
            plotSpeed(t, 0.f, std::max(1.f, beltSpeed * 1.2f));
        }

        void pushPanel(ImDrawList* dl) {
            ImGui::SeparatorText("Push");
            ImGui::SliderFloat("Push (N)", &pushForce, 0.f, 100.f, "%.0f");
            ImGui::Checkbox("Pushing (hold Space)", &pushing);
            Tracked& t = sim->tracked[0];
            const float n = PushMass * G;
            ImGui::Text("N = m g = %.1f N", double(n));
            ImGui::Text("Static friction holds up to muS N = %.1f N", double(PushStatic * n));
            ImGui::Text("Kinetic friction while sliding: muK N = %.1f N", double(PushKinetic * n));
            const bool moving = std::fabs(sim->speed(t)) > 1e-5f;
            if (!pushing) {
                ImGui::TextDisabled(moving ? "not pushed: sliding to a stop" : "not pushed: at rest");
            } else if (!moving) {
                ImGui::TextColored(ImVec4(0.5f, 0.9f, 0.55f, 1.f), "%s", pushForce <= PushStatic * n ? "held by static friction: should not move" : "starting to slide");
            } else {
                ImGui::Text("sliding: theory a = (F - muK N) / m = %.3f m/s^2", double((pushForce - PushKinetic * n) / PushMass));
            }
            if (pushing) {
                const V c = sim->world.getComponent<Component::Transform>(t.entity).position;
                drawArrow(dl, view.toScreen(c - V(0.6f + pushForce / 40.f, 0.f)), view.toScreen(c - V(0.62f, 0.f)), Color::Push);
            }
            plotSpeed(t, 0.f, 6.f);
        }

        // ---------------------------------------------------------------- frame

        void frame(double realSeconds) {
            const ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureKeyboard) {
                for (int i = 0; i < 4; ++i) {
                    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i))) {
                        scene = static_cast<Scene>(i);
                        reset();
                    }
                }
                if (ImGui::IsKeyPressed(ImGuiKey_R)) reset();
                if (ImGui::IsKeyPressed(ImGuiKey_P)) paused = !paused;
                if (scene == Scene::Push) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) pushing = true;
                    if (ImGui::IsKeyReleased(ImGuiKey_Space)) pushing = false;
                }
            }
            if (!paused) advance(realSeconds * double(timeScale));

            const ImVec2 sceneSize{io.DisplaySize.x - PanelWidth, io.DisplaySize.y};
            view.middle = {sceneSize.x * 0.5f, sceneSize.y * 0.5f};
            view.scale = scene == Scene::Slide ? (sceneSize.x - 40.f) / 46.f : scene == Scene::Slope ? 40.f : 30.f;
            if ((scene == Scene::Conveyor || scene == Scene::Push) && !sim->tracked.empty()) {  // follow the box
                const float x = sim->world.getComponent<Component::Transform>(sim->tracked[0].entity).position.x;
                view.center.x = std::clamp(x, 10.f, 30.f);
            }

            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            dl->AddRectFilled({0.f, 0.f}, io.DisplaySize, Color::Background);

            ImGui::SetNextWindowPos({io.DisplaySize.x - PanelWidth, 0.f});
            ImGui::SetNextWindowSize({PanelWidth, io.DisplaySize.y});
            ImGui::Begin("Friction demo", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::PushTextWrapPos(0.f);

            int sceneIndex = static_cast<int>(scene);
            const char* names[] = {"Slide (1)", "Slope (2)", "Conveyor (3)", "Push (4)"};
            for (int i = 0; i < 4; ++i) {
                if (i > 0) ImGui::SameLine();
                if (ImGui::RadioButton(names[i], &sceneIndex, i)) {
                    scene = static_cast<Scene>(i);
                    reset();
                }
            }
            if (ImGui::Button(paused ? "Run (P)" : "Pause (P)")) paused = !paused;
            ImGui::SameLine();
            if (ImGui::Button("Step 1/60 s")) {
                paused = true;
                stepOnce();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset (R)")) reset();
            ImGui::SliderFloat("Time scale", &timeScale, 0.1f, 2.f, "%.2f x");
            ImGui::Text("t = %.3f s, %d steps of 1/60 s", sim->time, sim->steps);

            // grounds first, boxes on top
            const float beltOffset = scene == Scene::Conveyor ? float(sim->time) * beltSpeed : -1.f;
            for (std::size_t i = 0; i < sim->grounds.size(); ++i) {
                const ImU32 top = scene == Scene::Slide ? sim->tracked[i].color : Color::GroundTop;
                drawGround(dl, view, sim->grounds[i], top, beltOffset);
            }
            switch (scene) {
                case Scene::Slide: slidePanel(dl); break;
                case Scene::Slope: slopePanel(); break;
                case Scene::Conveyor: conveyorPanel(); break;
                case Scene::Push: pushPanel(dl); break;
            }
            for (const Tracked& t : sim->tracked) drawBox(dl, view, *sim, t);

            ImGui::SeparatorText("About");
            ImGui::TextDisabled("Gravity, friction and integration are the engine's systems. The ground's support is the demo's own "
                                "stand-in (SupportSystem): the collision response does not exist yet.");
            ImGui::TextDisabled("Keys: 1-4 scenes, P pause, R reset, Space push (scene 4).");
            ImGui::PopTextWrapPos();
            ImGui::End();
        }
    };

    struct Options {
        const char* screenshot = nullptr;
        Scene scene = Scene::Slide;
        double seconds = 0.0;
        float push = -1.f;  // push force for the Push scene, -1: the default
    };

    Options parse(int argc, char** argv) {
        Options o;
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::strcmp(argv[i], "--screenshot") == 0) {
                o.screenshot = argv[++i];
            } else if (std::strcmp(argv[i], "--scene") == 0) {
                const char* s = argv[++i];
                o.scene = std::strcmp(s, "slope") == 0 ? Scene::Slope : std::strcmp(s, "conveyor") == 0 ? Scene::Conveyor
                        : std::strcmp(s, "push") == 0 ? Scene::Push : Scene::Slide;
            } else if (std::strcmp(argv[i], "--seconds") == 0) {
                o.seconds = std::atof(argv[++i]);
            } else if (std::strcmp(argv[i], "--push") == 0) {
                o.push = static_cast<float>(std::atof(argv[++i]));
            }
        }
        return o;
    }
}

int main(int argc, char** argv) {
    const Options options = parse(argc, argv);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("MetroV - friction demo", 1400, 820, SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!renderer) {
        std::fprintf(stderr, "window or renderer creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    Demo demo;
    demo.scene = options.scene;
    if (options.push >= 0.f) demo.pushForce = options.push;
    demo.reset();
    if (options.screenshot) {
        demo.paused = true;
        if (options.scene == Scene::Push) demo.pushing = true;
        for (int i = 0; i < static_cast<int>(options.seconds / Dt + 0.5); ++i) demo.stepOnce();
    }

    Uint64 last = SDL_GetTicksNS();
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
        const Uint64 now = SDL_GetTicksNS();
        const double seconds = std::min(0.1, double(now - last) / 1e9);  // a long pause (dragging the window) is not replayed
        last = now;

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        demo.frame(seconds);
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 22, 24, 29, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        if (options.screenshot && ++frames == 3) {
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
