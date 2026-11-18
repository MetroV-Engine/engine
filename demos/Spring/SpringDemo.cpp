// Visual check of SpringSystem: bodies on springs, moved by the engine's own
// systems (GravitySystem, SpringSystem, IntegrationSystem) at a fixed 60 Hz,
// next to what Hooke's law predicts.
//
// Scenes (keys 1 to 4, or the panel):
//   Oscillators  four weights hung from hooks (entities without RigidBody),
//                released from the same stretch, with damping ratios 0, 0.25,
//                1 (critical) and 2: period 2 pi sqrt(m / k), equilibrium
//                m g / k below the rest length, no overshoot from 1 on. Raise
//                k to see each one become unstable past
//                dt sqrt(k / m) = 2 (sqrt(1 + z^2) - z).
//   Rope         a weight on a stiff spring to a fixed point, swinging like a
//                pendulum: the damping acts along the spring only, so the
//                swing keeps its amplitude. Click to hook the rope elsewhere,
//                Space lets go, A / D push the weight.
//   Bridge       links joined by springs between two hooks: each link has two
//                springs, and the hooks carry the whole weight. Right click a
//                spring to cut it; remove a hook and its spring breaks.
//   Pair         two bodies joined by a spring, in space: each gets the
//                opposite of the other's force, so their momentum stays the
//                same and their center of mass moves in a straight line.
//
// In every scene, drag a body with the left button: the demo pulls it with a
// spring of its own (a Spring to a fixed point, moved to the cursor).
//
// Usage: demo_spring [--screenshot file.bmp] [--scene oscillators|rope|bridge|pair] [--seconds s] [--stiffness k]
//        --screenshot runs the scene for s seconds (default 0), saves the image and quits.
//        --stiffness sets k in the Oscillators and Bridge scenes.

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/Spring.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Physics/Gravity/GravitySystem.hpp"
#include "Engine/System/Physics/Integration/IntegrationSystem.hpp"
#include "Engine/System/Physics/Spring/SpringSystem.hpp"

using namespace Engine;
using V = Math::Vector2f;

namespace {
    constexpr double Dt = 1.0 / 60.0;
    constexpr float G = 9.81f;
    constexpr float Pi = 3.14159265f;
    constexpr float PanelWidth = 430.f;
    constexpr std::size_t HistorySize = 600;  // 10 s of samples at 60 Hz
    constexpr std::size_t TrailSize = 300;    // 5 s of positions
    constexpr float Noise = 1e-5f;            // float rounding on positions of a few metres is about 1e-6 m

    enum class Scene { Oscillators = 0, Rope = 1, Bridge = 2, Pair = 3 };

    namespace Color {
        const ImU32 Background = IM_COL32(22, 24, 29, 255);
        const ImU32 Grid = IM_COL32(32, 35, 42, 255);
        const ImU32 GridMajor = IM_COL32(44, 48, 57, 255);
        const ImU32 Text = IM_COL32(230, 232, 236, 255);
        const ImU32 Muted = IM_COL32(140, 146, 156, 255);
        const ImU32 Hook = IM_COL32(150, 152, 162, 255);
        const ImU32 HookPlate = IM_COL32(70, 72, 80, 255);
        const ImU32 SpringRest = IM_COL32(200, 202, 210, 255);
        const ImU32 Stretched = IM_COL32(240, 130, 70, 255);
        const ImU32 Compressed = IM_COL32(90, 160, 240, 255);
        const ImU32 Marker = IM_COL32(120, 230, 140, 255);
        const ImU32 Mouse = IM_COL32(250, 222, 96, 255);
        const ImU32 Warning = IM_COL32(240, 95, 95, 255);
        const ImU32 Link = IM_COL32(205, 150, 90, 255);
        const ImU32 Series[4] = {IM_COL32(100, 170, 240, 255), IM_COL32(240, 160, 70, 255), IM_COL32(110, 205, 135, 255),
                                 IM_COL32(195, 135, 235, 255)};
    }

    const ImVec4 Good{0.5f, 0.9f, 0.55f, 1.f};
    const ImVec4 Bad{0.95f, 0.4f, 0.4f, 1.f};

    /** How the demo draws an entity: a body (a disc) or a hook (a plate). Demo-only. */
    struct Look {
        float radius = 0.3f;
        ImU32 color = 0;
        bool hook = false;
    };

    /** The stability bound of semi-implicit Euler for a damped spring: dt * w < 2 (sqrt(1 + z^2) - z). */
    float stabilityLimit(float zeta) { return 2.f * (std::sqrt(1.f + zeta * zeta) - zeta); }

    /**
     * Period semi-implicit Euler gives at 60 Hz around the equilibrium, or -1 when it does not oscillate.
     * Each step multiplies (x, v dt) by [[1 - a, 1 - b], [-a, 1 - b]], with a = (w dt)^2 and
     * b = 2 z w dt; its eigenvalues turn by theta, with cos(theta) = (2 - a - b) / (2 sqrt(1 - b)).
     */
    float periodAt60Hz(float omega, float zeta) {
        const double a = double(omega) * double(omega) * Dt * Dt, b = 2.0 * double(zeta) * double(omega) * Dt;
        if (b >= 1.0) return -1.f;
        const double c = (2.0 - a - b) / (2.0 * std::sqrt(1.0 - b));
        if (c <= -1.0 || c >= 1.0) return -1.f;
        return float(2.0 * double(Pi) * Dt / std::acos(c));
    }

    float distanceToSegment(V p, V a, V b) {
        const V ab = b - a;
        const float t = std::clamp((p - a).dot(ab) / std::max(ab.dot(ab), 1e-9f), 0.f, 1.f);
        return (p - (a + ab * t)).length();
    }

    // ------------------------------------------------------------------ simulation

    struct Simulation {
        ECS::Registry world;
        ECS::SystemManager systems;
        double time = 0.0;
        int steps = 0;
        int blownUp = 0;

        explicit Simulation(V gravity) {
            world.setResource<Resource::GravitySettings>(Resource::GravitySettings{gravity, Physic::GravitationalConstant<float>});
            systems.addSystem<System::GravitySystem>(world);
            systems.addSystem<System::SpringSystem>(world);
            systems.addSystem<System::IntegrationSystem>(world);
        }

        ECS::Entity body(V position, float mass, ImU32 color, V velocity = V::zero(), float radius = 0.3f) {
            const ECS::Entity e = world.spawnEntity();
            world.addComponent(e, Component::Transform{position});
            Component::RigidBody rigid;
            rigid.mass = mass;
            rigid.velocity = velocity;
            world.addComponent(e, rigid);
            world.addComponent(e, Look{radius, color, false});
            return e;
        }

        /** An end that holds springs without moving: a Transform, no RigidBody. */
        ECS::Entity hook(V position) {
            const ECS::Entity e = world.spawnEntity("hook");
            world.addComponent(e, Component::Transform{position});
            world.addComponent(e, Look{0.12f, Color::Hook, true});
            return e;
        }

        /** Springs live on entities of their own. */
        ECS::Entity spring(ECS::Entity body, ECS::Entity other, V anchor, float rest, float stiffness, float damping) {
            Component::Spring s;
            s.body = body;
            s.other = other;
            s.anchor = anchor;
            s.restLength = rest;
            s.stiffness = stiffness;
            s.damping = damping;
            const ECS::Entity e = world.spawnEntity("spring");
            world.addComponent(e, s);
            return e;
        }

        [[nodiscard]] bool alive(ECS::Entity e) { return world.hasComponents<Component::Transform>(e); }
        [[nodiscard]] V position(ECS::Entity e) { return world.getComponent<Component::Transform>(e).position; }
        [[nodiscard]] Component::RigidBody& rigid(ECS::Entity e) { return world.getComponent<Component::RigidBody>(e); }
        [[nodiscard]] bool isHook(ECS::Entity e) { return world.hasComponents<Look>(e) && world.getComponent<Look>(e).hook; }

        void step() {
            systems.update(world, Dt);
            time += Dt;
            ++steps;
            // A body flung away by an unstable spring is removed: its springs then break.
            std::vector<ECS::Entity> gone;
            for (auto [e, transform, rigid] : world.view<Component::Transform, Component::RigidBody>()) {
                const V p = transform.position;
                if (!std::isfinite(p.x) || !std::isfinite(p.y) || std::fabs(p.x) > 1e4f || std::fabs(p.y) > 1e4f) {
                    gone.push_back(e);
                }
            }
            for (const ECS::Entity e : gone) {
                world.killEntity(e);
                ++blownUp;
            }
        }
    };

    /** The two ends of a spring as SpringSystem sees them, or nothing when the spring is broken. */
    struct Ends {
        V body;
        V other;
        V relativeVelocity;
    };

    std::optional<Ends> endsOf(Simulation& sim, const Component::Spring& s) {
        const bool toPoint = s.other == ECS::Entity{};
        if (!sim.alive(s.body) || (!toPoint && !sim.alive(s.other))) {
            return std::nullopt;
        }
        const auto velocity = [&](ECS::Entity e) {
            return sim.world.hasComponents<Component::RigidBody>(e) ? sim.rigid(e).velocity : V::zero();
        };
        return Ends{sim.position(s.body), toPoint ? s.anchor : sim.position(s.other),
                    velocity(s.body) - (toPoint ? V::zero() : velocity(s.other))};
    }

    /** The force the spring puts on its `body` end; the other end gets the opposite. */
    V forceOnBody(const Component::Spring& s, const Ends& ends) {
        return Physic::dampedSpringForce(ends.body, ends.other, ends.relativeVelocity, s.restLength, s.stiffness, s.damping);
    }

    // ------------------------------------------------------------------ view and drawing

    struct View {
        V center;
        float scale = 40.f;  // pixels per metre
        ImVec2 middle;
        [[nodiscard]] ImVec2 toScreen(V w) const { return {middle.x + (w.x - center.x) * scale, middle.y - (w.y - center.y) * scale}; }
        [[nodiscard]] V toWorld(ImVec2 s) const { return {center.x + (s.x - middle.x) / scale, center.y - (s.y - middle.y) / scale}; }
    };

    ImU32 mix(ImU32 a, ImU32 b, float t) {
        const ImVec4 x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
        return ImGui::ColorConvertFloat4ToU32({x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t, x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t});
    }

    ImU32 withAlpha(ImU32 color, int alpha) { return (color & ~IM_COL32_A_MASK) | (ImU32(alpha) << IM_COL32_A_SHIFT); }

    /** Orange when stretched, blue when compressed, gray at rest length. */
    ImU32 springColor(float length, float rest) {
        const float strain = (length - rest) / std::max(rest, 0.2f);
        return mix(Color::SpringRest, strain > 0.f ? Color::Stretched : Color::Compressed, std::min(std::fabs(strain) / 0.25f, 1.f));
    }

    void drawGrid(ImDrawList* dl, const View& view, ImVec2 size) {
        const V low = view.toWorld({0.f, size.y}), high = view.toWorld({size.x, 0.f});
        if (view.scale < 8.f) return;
        for (float x = std::floor(low.x); x <= high.x; x += 1.f) {
            const ImU32 c = std::lround(x) % 5 == 0 ? Color::GridMajor : Color::Grid;
            dl->AddLine(view.toScreen(V(x, low.y)), view.toScreen(V(x, high.y)), c);
        }
        for (float y = std::floor(low.y); y <= high.y; y += 1.f) {
            const ImU32 c = std::lround(y) % 5 == 0 ? Color::GridMajor : Color::Grid;
            dl->AddLine(view.toScreen(V(low.x, y)), view.toScreen(V(high.x, y)), c);
        }
    }

    void drawSpring(ImDrawList* dl, const View& view, V a, V b, float rest, ImU32 color) {
        const V d = b - a;
        const float length = d.length();
        if (length < 1e-4f) return;
        const V u = d / length, n{-u.y, u.x};
        const int zigs = 2 * std::clamp(int(rest / 0.25f), 4, 24);
        const float lead = std::min(0.15f, length * 0.1f), width = 0.13f;
        std::vector<ImVec2> points{view.toScreen(a), view.toScreen(a + u * lead)};
        for (int i = 0; i < zigs; ++i) {
            const float t = lead + (length - 2.f * lead) * (float(i) + 0.5f) / float(zigs);
            points.push_back(view.toScreen(a + u * t + n * (i % 2 == 0 ? width : -width)));
        }
        points.push_back(view.toScreen(b - u * lead));
        points.push_back(view.toScreen(b));
        dl->AddPolyline(points.data(), int(points.size()), color, ImDrawFlags_None, 2.f);
    }

    void drawDashed(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color, float thickness = 1.5f) {
        const float dx = b.x - a.x, dy = b.y - a.y, length = std::sqrt(dx * dx + dy * dy);
        for (float s = 0.f; s < length; s += 12.f) {
            const float e = std::min(s + 7.f, length);
            dl->AddLine({a.x + dx * s / length, a.y + dy * s / length}, {a.x + dx * e / length, a.y + dy * e / length}, color, thickness);
        }
    }

    void drawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color, float thickness = 3.f) {
        dl->AddLine(from, to, color, thickness);
        const float dx = to.x - from.x, dy = to.y - from.y, length = std::sqrt(dx * dx + dy * dy);
        if (length < 1.f) return;
        const float ux = dx / length, uy = dy / length, head = 10.f;
        dl->AddTriangleFilled(to, {to.x - ux * head - uy * head * 0.5f, to.y - uy * head + ux * head * 0.5f},
                              {to.x - ux * head + uy * head * 0.5f, to.y - uy * head - ux * head * 0.5f}, color);
    }

    void drawPoint(ImDrawList* dl, const View& view, V p) {
        const ImVec2 s = view.toScreen(p);
        dl->AddCircle(s, 6.f, Color::Hook, 0, 2.f);
        dl->AddLine({s.x - 9.f, s.y}, {s.x + 9.f, s.y}, Color::Hook, 1.5f);
        dl->AddLine({s.x, s.y - 9.f}, {s.x, s.y + 9.f}, Color::Hook, 1.5f);
    }

    void textCentered(ImDrawList* dl, ImVec2 at, ImU32 color, const char* text) {
        const ImVec2 size = ImGui::CalcTextSize(text);
        dl->AddText({at.x - size.x * 0.5f, at.y - size.y * 0.5f}, color, text);
    }

    /** Several series over the same time window, newest on the right. */
    void plotSeries(const char* id, const std::vector<const std::vector<float>*>& series, const std::vector<ImU32>& colors, float low,
                    float high, float height) {
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        const float width = ImGui::GetContentRegionAvail().x;
        ImGui::InvisibleButton(id, {width, height});
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(origin, {origin.x + width, origin.y + height}, IM_COL32(30, 33, 40, 255), 3.f);
        const auto y = [&](float v) { return origin.y + height * (high - std::clamp(v, low, high)) / (high - low); };
        dl->AddLine({origin.x, y(0.f)}, {origin.x + width, y(0.f)}, Color::GridMajor);
        std::vector<ImVec2> points;
        for (std::size_t s = 0; s < series.size(); ++s) {
            const std::vector<float>& values = *series[s];
            points.clear();
            const std::size_t offset = HistorySize - values.size();
            for (std::size_t i = 0; i < values.size(); ++i) {
                points.push_back({origin.x + width * float(offset + i) / float(HistorySize - 1), y(values[i])});
            }
            if (points.size() > 1) dl->AddPolyline(points.data(), int(points.size()), colors[s], ImDrawFlags_None, 1.5f);
        }
    }

    void push(std::vector<float>& history, float value) {
        if (history.size() == HistorySize) history.erase(history.begin());
        history.push_back(value);
    }

    // ------------------------------------------------------------------ the demo

    /** One of the four weights of the Oscillators scene. */
    struct Oscillator {
        ECS::Entity hook, body;
        float zeta = 0.f;
        const char* label = "";
        ImU32 color = 0;
        std::vector<float> history;       // displacement from the equilibrium, m
        std::vector<double> upCrossings;  // times it crossed the equilibrium going up
        float last = 0.f;                 // displacement at the previous step
        float peak = 0.f;                 // largest |displacement| since the last up crossing
        float highest = -1e9f;            // highest displacement above the equilibrium
        double blewUpAt = -1.0;
    };

    /** The spring the mouse pulls a body with. */
    struct Drag {
        ECS::Entity spring;
        ECS::Entity body;
    };

    struct Demo {
        Scene scene = Scene::Oscillators;
        std::unique_ptr<Simulation> sim;
        View view;
        ImVec2 sceneSize;
        bool paused = false;
        float timeScale = 1.f;
        double accumulator = 0.0;
        std::optional<Drag> drag;

        // Oscillators
        static constexpr float OscRest = 2.f;
        float oscStiffness = 40.f, oscMass = 1.f, oscStart = 1.f;
        std::vector<Oscillator> oscillators;

        // Rope
        static constexpr float RopeMass = 70.f, RopeRest = 4.f, RopeStiffnessPerKg = 400.f;
        float ropeAngle = 30.f;
        ECS::Entity ropeBody, ropeSpring;  // ropeSpring: no entity once let go
        int pushDirection = 0;
        std::vector<double> swingCrossings;  // times the angle crossed 0 going right
        float lastAngle = 0.f, swingPeak = 0.f, swingAmplitude = -1.f, swingStart = -1.f, bottomTension = -1.f;
        bool measuring = true;

        // Bridge
        static constexpr int BridgeLinks = 9;
        static constexpr float BridgeMass = 2.f, BridgeRest = 1.f, BridgeSpan = 12.f;
        float bridgeStiffness = 600.f, bridgeDamping = 20.f;
        ECS::Entity leftHook, rightHook;
        int cuts = 0;

        // Pair
        float pairMass1 = 1.f, pairMass2 = 3.f, pairStiffness = 30.f, pairDamping = 0.f;
        static constexpr float PairRest = 3.f;
        ECS::Entity pairA, pairB, pairSpring;
        V momentum0, center0, centerVelocity;
        double time0 = 0.0;
        float energy0 = 0.f;
        std::deque<V> trailA, trailB, trailCenter;

        // ---------------------------------------------------------------- setup

        void reset() {
            drag.reset();
            accumulator = 0.0;
            switch (scene) {
                case Scene::Oscillators: resetOscillators(); break;
                case Scene::Rope: resetRope(); break;
                case Scene::Bridge: resetBridge(); break;
                case Scene::Pair: resetPair(); break;
            }
        }

        [[nodiscard]] float oscEquilibrium() const { return -(OscRest + oscMass * G / oscStiffness); }

        void resetOscillators() {
            sim = std::make_unique<Simulation>(V(0.f, -G));
            oscillators.clear();
            const float zetas[] = {0.f, 0.25f, 1.f, 2.f};
            const char* labels[] = {"undamped, z = 0", "underdamped, z = 0.25", "critical, z = 1", "overdamped, z = 2"};
            for (int i = 0; i < 4; ++i) {
                const float x = -4.5f + 3.f * float(i);
                Oscillator o;
                o.zeta = zetas[i];
                o.label = labels[i];
                o.color = Color::Series[i];
                o.hook = sim->hook(V(x, 0.f));
                o.body = sim->body(V(x, oscEquilibrium() - oscStart), oscMass, o.color, V::zero(), 0.2f + 0.1f * std::cbrt(oscMass));
                // body first, the hook as the other end: the hook has no RigidBody, so only the weight moves
                sim->spring(o.body, o.hook, V::zero(), OscRest, oscStiffness, o.zeta * Physic::criticalDamping(oscStiffness, oscMass));
                o.last = -oscStart;
                oscillators.push_back(o);
            }
        }

        void resetRope() {
            sim = std::make_unique<Simulation>(V(0.f, -G));
            const float k = RopeStiffnessPerKg * RopeMass, a = ropeAngle * Pi / 180.f;
            const float length = RopeRest + RopeMass * G / k;  // how long it hangs at rest
            ropeBody = sim->body(V(std::sin(a), -std::cos(a)) * length, RopeMass, Color::Series[1], V::zero(), 0.35f);
            ropeSpring = sim->spring(ropeBody, {}, V::zero(), RopeRest, k, Physic::criticalDamping(k, RopeMass));
            view.center = V(0.f, -2.5f);
            restartSwing(a);
        }

        void resetBridge() {
            sim = std::make_unique<Simulation>(V(0.f, -G));
            leftHook = sim->hook(V(-BridgeSpan / 2.f, 0.f));
            rightHook = sim->hook(V(BridgeSpan / 2.f, 0.f));
            ECS::Entity previous = leftHook;
            for (int i = 1; i <= BridgeLinks; ++i) {
                const float x = -BridgeSpan / 2.f + BridgeSpan * float(i) / float(BridgeLinks + 1);
                const ECS::Entity link = sim->body(V(x, 0.f), BridgeMass, Color::Link, V::zero(), 0.22f);
                sim->spring(link, previous, {}, BridgeRest, bridgeStiffness, bridgeDamping);
                previous = link;
            }
            sim->spring(rightHook, previous, {}, BridgeRest, bridgeStiffness, bridgeDamping);  // starts from the hook: works too
            cuts = 0;
            // Start at rest in the shape it hangs in. The damping acts along the springs only, so a bridge
            // dropped straight bounces up and down for about a minute: let it settle first, with extra
            // damping (demo-only), then start the clock.
            for (int i = 0; i < 1200; ++i) {
                sim->step();
                for (auto [e, rigid] : sim->world.view<Component::RigidBody>()) rigid.velocity = rigid.velocity * 0.95f;
            }
            for (auto [e, rigid] : sim->world.view<Component::RigidBody>()) rigid.velocity = V::zero();
            sim->time = 0.0;
            sim->steps = 0;
        }

        void resetPair() {
            sim = std::make_unique<Simulation>(V::zero());
            const float total = pairMass1 + pairMass2;
            const V velocity(1.f, 0.f), relative(0.f, 4.f), separation(4.f, 0.f);  // spinning, stretched by 1 m
            pairA = sim->body(separation * (-pairMass2 / total), pairMass1, Color::Series[0], velocity + relative * (pairMass2 / total),
                              0.2f + 0.12f * std::cbrt(pairMass1));
            pairB = sim->body(separation * (pairMass1 / total), pairMass2, Color::Series[1], velocity - relative * (pairMass1 / total),
                              0.2f + 0.12f * std::cbrt(pairMass2));
            pairSpring = sim->spring(pairA, pairB, {}, PairRest, pairStiffness, pairDamping);
            trailA.clear();
            trailB.clear();
            trailCenter.clear();
            rebaselinePair();
        }

        // ---------------------------------------------------------------- stepping

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
            if (scene == Scene::Rope && pushDirection != 0 && sim->alive(ropeBody)) {
                // A push is a force like any other: added before the step's systems run.
                sim->rigid(ropeBody).force += V(float(pushDirection) * 0.3f * RopeMass * G, 0.f);
            }
            sim->step();
            switch (scene) {
                case Scene::Oscillators: trackOscillators(); break;
                case Scene::Rope: trackRope(); break;
                case Scene::Bridge: break;
                case Scene::Pair: trackPair(); break;
            }
        }

        void trackOscillators() {
            for (Oscillator& o : oscillators) {
                if (o.blewUpAt >= 0.0) continue;
                const float d = sim->alive(o.body) ? sim->position(o.body).y - oscEquilibrium() : 1e9f;
                if (!std::isfinite(d) || std::fabs(d) > 5.f * oscStart + 1.f) {  // growing: unstable
                    if (sim->alive(o.body)) sim->world.killEntity(o.body);  // its spring breaks
                    o.blewUpAt = sim->time;
                    continue;
                }
                push(o.history, d);
                o.peak = std::max(o.peak, std::fabs(d));
                if (o.last < 0.f && d >= 0.f) {
                    // once settled, rounding makes it cross back and forth: those are not swings
                    if (o.peak > Noise) o.upCrossings.push_back(sim->time - Dt + Dt * double(-o.last / (d - o.last)));
                    o.peak = 0.f;
                }
                o.highest = std::max(o.highest, d);
                o.last = d;
            }
        }

        [[nodiscard]] bool roped() { return ropeSpring != ECS::Entity{} && sim->world.hasComponents<Component::Spring>(ropeSpring); }

        /** Angle of the rope from straight down, positive to the right. */
        [[nodiscard]] float ropeAngleNow() {
            const V d = sim->position(ropeBody) - sim->world.getComponent<Component::Spring>(ropeSpring).anchor;
            return std::atan2(d.x, -d.y);
        }

        void restartSwing(float amplitude) {
            swingCrossings.clear();
            swingAmplitude = amplitude;
            swingStart = amplitude;
            swingPeak = 0.f;
            bottomTension = -1.f;
            measuring = true;
            if (sim->alive(ropeBody) && roped()) lastAngle = ropeAngleNow();
        }

        void trackRope() {
            if (!sim->alive(ropeBody)) return;
            if (sim->position(ropeBody).y < -60.f) {  // fell out of the world
                reset();
                return;
            }
            if (!roped() || pushDirection != 0) {
                measuring = false;
                return;
            }
            if (!measuring) restartSwing(-1.f);  // amplitude unknown until the next swing
            const float a = ropeAngleNow();
            if ((lastAngle < 0.f) != (a < 0.f)) {  // through the bottom
                if (lastAngle < 0.f) swingCrossings.push_back(sim->time - Dt + Dt * double(-lastAngle / (a - lastAngle)));
                if (swingPeak > 0.f) swingAmplitude = swingPeak;
                swingPeak = 0.f;
                const Component::Spring& s = sim->world.getComponent<Component::Spring>(ropeSpring);
                if (const auto ends = endsOf(*sim, s)) bottomTension = forceOnBody(s, *ends).length();
            }
            swingPeak = std::max(swingPeak, std::fabs(a));
            lastAngle = a;
        }

        [[nodiscard]] V pairMomentum() { return sim->rigid(pairA).velocity * pairMass1 + sim->rigid(pairB).velocity * pairMass2; }
        [[nodiscard]] V pairCenter() {
            return (sim->position(pairA) * pairMass1 + sim->position(pairB) * pairMass2) / (pairMass1 + pairMass2);
        }
        [[nodiscard]] float pairEnergy() {
            const V va = sim->rigid(pairA).velocity, vb = sim->rigid(pairB).velocity;
            const float stretch = (sim->position(pairB) - sim->position(pairA)).length() - PairRest;
            return 0.5f * (pairMass1 * va.dot(va) + pairMass2 * vb.dot(vb) + pairStiffness * stretch * stretch);
        }

        void rebaselinePair() {
            momentum0 = pairMomentum();
            center0 = pairCenter();
            centerVelocity = momentum0 / (pairMass1 + pairMass2);
            time0 = sim->time;
            energy0 = pairEnergy();
        }

        void trackPair() {
            if (!sim->alive(pairA) || !sim->alive(pairB)) return;
            const auto record = [](std::deque<V>& trail, V p) {
                if (trail.size() == TrailSize) trail.pop_front();
                trail.push_back(p);
            };
            record(trailA, sim->position(pairA));
            record(trailB, sim->position(pairB));
            record(trailCenter, pairCenter());
        }

        // ---------------------------------------------------------------- mouse

        std::optional<ECS::Entity> bodyAt(V point) {
            std::optional<ECS::Entity> best;
            float bestDistance = 1e9f;
            for (auto [e, transform, rigid, look] : sim->world.view<Component::Transform, Component::RigidBody, Look>()) {
                const float d = (transform.position - point).length();
                if (d < look.radius + 0.25f && d < bestDistance) {
                    best = e;
                    bestDistance = d;
                }
            }
            return best;
        }

        void startDrag(ECS::Entity body, V point) {
            const float mass = sim->rigid(body).mass, k = 80.f * mass;
            drag = Drag{sim->spring(body, {}, point, 0.f, k, Physic::criticalDamping(k, mass)), body};
        }

        void endDrag() {
            if (sim->world.hasComponents<Component::Spring>(drag->spring)) sim->world.killEntity(drag->spring);
            drag.reset();
            if (scene == Scene::Pair && sim->alive(pairA) && sim->alive(pairB)) rebaselinePair();  // the outside pull is gone
        }

        void grapple(V point) {
            if (!sim->alive(ropeBody)) return;
            const float length = (sim->position(ropeBody) - point).length();
            if (length < 0.5f) return;
            const float k = RopeStiffnessPerKg * RopeMass;
            if (roped()) {
                Component::Spring& s = sim->world.getComponent<Component::Spring>(ropeSpring);
                s.anchor = point;
                s.restLength = length;
            } else {
                ropeSpring = sim->spring(ropeBody, {}, point, length, k, Physic::criticalDamping(k, RopeMass));
            }
            restartSwing(-1.f);
        }

        void cutAt(V point) {
            std::optional<ECS::Entity> best;
            float bestDistance = 0.3f;
            for (auto [e, s] : sim->world.view<Component::Spring>()) {
                if (drag && e == drag->spring) continue;
                if (const auto ends = endsOf(*sim, s)) {
                    const float d = distanceToSegment(point, ends->body, ends->other);
                    if (d < bestDistance) {
                        best = e;
                        bestDistance = d;
                    }
                }
            }
            if (best) {
                sim->world.killEntity(*best);
                ++cuts;
            }
        }

        void handleMouse() {
            const ImGuiIO& io = ImGui::GetIO();
            const V mouse = view.toWorld(io.MousePos);
            if (drag) {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !sim->alive(drag->body)) {
                    endDrag();
                } else {
                    sim->world.getComponent<Component::Spring>(drag->spring).anchor = mouse;
                }
            }
            if (io.WantCaptureMouse || io.MousePos.x >= sceneSize.x) return;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (const auto body = bodyAt(mouse)) {
                    startDrag(*body, mouse);
                } else if (scene == Scene::Rope) {
                    grapple(mouse);
                }
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && scene == Scene::Bridge) cutAt(mouse);
        }

        // ---------------------------------------------------------------- drawing the world

        void drawSprings(ImDrawList* dl) {
            for (auto [e, s] : sim->world.view<Component::Spring>()) {
                const auto ends = endsOf(*sim, s);
                if (!ends) continue;  // broken: an end was killed
                if (drag && e == drag->spring) {
                    dl->AddLine(view.toScreen(ends->body), view.toScreen(ends->other), Color::Mouse, 1.5f);
                    dl->AddCircleFilled(view.toScreen(ends->other), 4.f, Color::Mouse);
                    continue;
                }
                drawSpring(dl, view, ends->body, ends->other, s.restLength, springColor((ends->body - ends->other).length(), s.restLength));
                if (s.other == ECS::Entity{}) drawPoint(dl, view, s.anchor);
            }
        }

        void drawBodies(ImDrawList* dl) {
            for (auto [e, transform, look] : sim->world.view<Component::Transform, Look>()) {
                const ImVec2 p = view.toScreen(transform.position);
                if (look.hook) {
                    const ImVec2 a = view.toScreen(transform.position + V(-0.35f, 0.32f)), b = view.toScreen(transform.position + V(0.35f, 0.12f));
                    dl->AddRectFilled(a, b, Color::HookPlate);
                    dl->AddLine(p, {p.x, b.y}, Color::Hook, 2.f);
                    dl->AddCircle(p, std::max(4.f, look.radius * view.scale), Color::Hook, 0, 2.f);
                } else {
                    const float r = std::max(4.f, look.radius * view.scale);
                    dl->AddCircleFilled(p, r, look.color);
                    dl->AddCircle(p, r, IM_COL32(0, 0, 0, 140), 0, 1.5f);
                }
            }
        }

        // ---------------------------------------------------------------- per scene: scene overlay and panel

        void oscillatorsScene(ImDrawList* dl) {
            const float eq = oscEquilibrium();
            drawDashed(dl, view.toScreen(V(-6.3f, eq)), view.toScreen(V(6.3f, eq)), Color::Marker);
            const ImVec2 p = view.toScreen(V(-6.3f, eq));
            dl->AddText({p.x, p.y - 18.f}, Color::Marker, "equilibrium");
            for (const Oscillator& o : oscillators) {
                const ImVec2 h = view.toScreen(sim->position(o.hook));
                textCentered(dl, {h.x, h.y - 0.32f * view.scale - 12.f}, Color::Text, o.label);
                if (o.blewUpAt >= 0.0) textCentered(dl, {h.x, h.y + 30.f}, Color::Warning, "UNSTABLE: blew up");
            }
        }

        void oscillatorsPanel() {
            ImGui::SeparatorText("Oscillators");
            bool changed = ImGui::SliderFloat("k (N/m)", &oscStiffness, 5.f, 20000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("m (kg)", &oscMass, 0.1f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("Pulled down by (m)", &oscStart, 0.1f, 1.5f, "%.2f");
            if (changed) reset();
            const float omega = std::sqrt(oscStiffness / oscMass);
            ImGui::Text("Equilibrium %.3f m under the hooks: L0 + m g / k = %.3f m", double(-oscEquilibrium()),
                        double(OscRest + oscMass * G / oscStiffness));
            ImGui::Text("w0 = sqrt(k / m) = %.2f rad/s, dt w0 = %.3f", double(omega), double(omega) * Dt);
            ImGui::TextDisabled("Each is stable while dt w0 < 2 (sqrt(1 + z^2) - z), with z = c / (2 sqrt(k m)).");

            std::vector<const std::vector<float>*> series;
            std::vector<ImU32> colors;
            for (const Oscillator& o : oscillators) {
                const float c = o.zeta * Physic::criticalDamping(oscStiffness, oscMass);
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(o.color), "%s   (c = %.2f N s/m)", o.label, double(c));
                const float limit = stabilityLimit(o.zeta);
                const bool stable = omega * float(Dt) < limit;
                ImGui::TextColored(stable ? Good : Bad, "  stable while dt w0 < %.3f: %s", double(limit), stable ? "stable" : "UNSTABLE");
                if (o.blewUpAt >= 0.0) {
                    ImGui::TextColored(Bad, "  blew up after %.2f s", o.blewUpAt);
                    continue;
                }
                series.push_back(&o.history);
                colors.push_back(o.color);
                if (o.zeta < 1.f) {
                    const float theory = 2.f * Pi / (omega * std::sqrt(1.f - o.zeta * o.zeta));
                    const float discrete = periodAt60Hz(omega, o.zeta);
                    if (o.upCrossings.size() >= 2) {
                        const double measured = (o.upCrossings.back() - o.upCrossings.front()) / double(o.upCrossings.size() - 1);
                        ImGui::Text("  period %.4f s   at 60 Hz %.4f s   theory %.4f s", measured, double(discrete), double(theory));
                    } else {
                        ImGui::TextDisabled("  period: measuring...   theory %.4f s", double(theory));
                    }
                } else if (o.highest > Noise) {
                    ImGui::TextColored(Bad, "  went %.3f mm past the equilibrium (overshoot)", double(o.highest) * 1000.0);
                } else {
                    ImGui::TextColored(Good, "  never went past the equilibrium (to 0.01 mm)");
                }
                ImGui::Text("  now %+.4f m from the equilibrium", double(o.last));
            }
            ImGui::TextDisabled("Displacement from the equilibrium, last 10 s:");
            plotSeries("##displacement", series, colors, -oscStart * 1.15f, oscStart * 1.15f, 110.f);
            ImGui::TextDisabled("theory: 2 pi / (w0 sqrt(1 - z^2)); at 60 Hz: what semi-implicit Euler gives (see periodAt60Hz). "
                                "Raise k: the overdamped weight blows up first (k > %.0f), then the critical one (%.0f), "
                                "the undamped one last (%.0f).",
                                double(std::pow(stabilityLimit(2.f) * 60.f, 2.f) * oscMass), double(std::pow(stabilityLimit(1.f) * 60.f, 2.f) * oscMass),
                                double(std::pow(stabilityLimit(0.f) * 60.f, 2.f) * oscMass));
        }

        void ropeScene(ImDrawList* dl) {
            if (!sim->alive(ropeBody) || !roped()) return;
            const Component::Spring& s = sim->world.getComponent<Component::Spring>(ropeSpring);
            const ImVec2 a = view.toScreen(s.anchor);
            const float hanging = s.restLength + RopeMass * G / s.stiffness;
            dl->AddCircle(a, hanging * view.scale, withAlpha(Color::Muted, 60), 128, 1.f);
            drawDashed(dl, a, view.toScreen(s.anchor - V(0.f, hanging + 0.5f)), withAlpha(Color::Muted, 140));
            if (pushDirection != 0) {
                const V c = sim->position(ropeBody);
                const float dir = float(pushDirection);
                drawArrow(dl, view.toScreen(c - V(dir * 1.6f, 0.f)), view.toScreen(c - V(dir * 0.4f, 0.f)), Color::Mouse);
            }
        }

        void ropePanel() {
            ImGui::SeparatorText("Rope");
            if (ImGui::SliderFloat("Start angle (degrees)", &ropeAngle, 5.f, 80.f, "%.0f")) reset();
            const float k = RopeStiffnessPerKg * RopeMass;
            ImGui::Text("%.0f kg on a spring of k = %.0f N/m (%.0f m), critically damped", double(RopeMass), double(k), double(RopeStiffnessPerKg));
            if (!sim->alive(ropeBody)) return;
            if (!roped()) {
                ImGui::TextColored(Good, "Let go: falling. Click to hook the rope somewhere.");
                return;
            }
            const Component::Spring& s = sim->world.getComponent<Component::Spring>(ropeSpring);
            const auto ends = endsOf(*sim, s);
            if (!ends) return;
            const float length = (ends->body - ends->other).length(), tension = forceOnBody(s, *ends).length();
            ImGui::Text("Length %.3f m for a rest length of %.3f m: stretched %.1f mm", double(length), double(s.restLength),
                        double(length - s.restLength) * 1000.0);
            ImGui::Text("Tension %.0f N, weight m g = %.0f N", double(tension), double(RopeMass * G));
            ImGui::Text("Angle %+.1f degrees", double(ropeAngleNow() * 180.f / Pi));
            if (pushDirection != 0) {
                ImGui::TextColored(Good, "Pushed: measurements start again when you let go of A / D.");
                return;
            }
            if (swingAmplitude < 0.f) {
                ImGui::TextDisabled("Swing amplitude: measuring...");
                return;
            }
            const float amplitude = swingAmplitude;
            if (swingStart > 0.f) {
                ImGui::Text("Swing amplitude %.2f degrees, started at %.2f", double(amplitude * 180.f / Pi), double(swingStart * 180.f / Pi));
            } else {
                ImGui::Text("Swing amplitude %.2f degrees", double(amplitude * 180.f / Pi));
            }
            ImGui::TextDisabled("The damping acts along the rope only: it stops the bouncing, not the swing.");
            const float hanging = s.restLength + RopeMass * G / k;
            const float theory = 2.f * Pi * std::sqrt(hanging / G) * (1.f + amplitude * amplitude / 16.f);
            if (swingCrossings.size() >= 2) {
                const double measured = (swingCrossings.back() - swingCrossings.front()) / double(swingCrossings.size() - 1);
                ImGui::Text("Period %.3f s, theory %.3f s", measured, double(theory));
            } else {
                ImGui::TextDisabled("Period: measuring...   theory %.3f s", double(theory));
            }
            ImGui::TextDisabled("theory: 2 pi sqrt(L / g) (1 + a^2 / 16) for a swing of amplitude a, L = rest + m g / k");
            const float bottomTheory = RopeMass * G * (3.f - 2.f * std::cos(amplitude));
            if (bottomTension >= 0.f) {
                ImGui::Text("Tension at the bottom %.0f N, theory m g (3 - 2 cos a) = %.0f N", double(bottomTension), double(bottomTheory));
            }
        }

        void bridgeScene(ImDrawList* dl) {
            // what the springs pull each hook with: they carry the bridge
            for (const ECS::Entity hook : {leftHook, rightHook}) {
                if (!sim->alive(hook)) continue;
                V pull = V::zero();
                for (auto [e, s] : sim->world.view<Component::Spring>()) {
                    if (s.body != hook && s.other != hook) continue;
                    if (const auto ends = endsOf(*sim, s)) pull += s.body == hook ? forceOnBody(s, *ends) : forceOnBody(s, *ends) * -1.f;
                }
                const V at = sim->position(hook);
                drawArrow(dl, view.toScreen(at), view.toScreen(at + pull * 0.02f), Color::Marker, 2.5f);
                char text[48];
                std::snprintf(text, sizeof text, "%.1f N", double(pull.length()));
                const ImVec2 p = view.toScreen(at + V(0.f, 0.7f));
                textCentered(dl, p, Color::Marker, text);
            }
        }

        void bridgePanel() {
            ImGui::SeparatorText("Bridge");
            bool changed = ImGui::SliderFloat("k (N/m)", &bridgeStiffness, 50.f, 5000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("c (N s/m)", &bridgeDamping, 0.f, 60.f, "%.1f");
            if (changed) reset();
            ImGui::Text("%d links of %.0f kg, rest length %.1f m, hooks %.0f m apart", BridgeLinks, double(BridgeMass), double(BridgeRest),
                        double(BridgeSpan));
            int springs = 0, broken = 0, links = 0;
            V hookPull = V::zero();
            float kinetic = 0.f;
            for (auto [e, s] : sim->world.view<Component::Spring>()) {
                if (drag && e == drag->spring) continue;
                ++springs;
                const auto ends = endsOf(*sim, s);
                if (!ends) {
                    ++broken;
                    continue;
                }
                const V f = forceOnBody(s, *ends);
                if (sim->isHook(s.body)) hookPull += f;
                if (sim->isHook(s.other)) hookPull -= f;
            }
            for (auto [e, rigid] : sim->world.view<Component::RigidBody>()) {
                ++links;
                kinetic += 0.5f * rigid.mass * rigid.velocity.dot(rigid.velocity);
            }
            const float weight = float(links) * BridgeMass * G;
            ImGui::Text("The springs pull the hooks down with %.2f N", double(-hookPull.y));
            ImGui::Text("The %d links weigh %.2f N", links, double(weight));
            ImGui::TextDisabled("At rest they are equal: the hooks carry the whole bridge (the difference is what still accelerates). "
                                "It starts at rest in the shape it hangs in; drag a link or cut a spring to set it moving.");
            ImGui::Text("Kinetic energy %.4f J", double(kinetic));
            ImGui::Text("Springs: %d, cut: %d, broken (an end was removed): %d", springs, cuts, broken);
            if (ImGui::Button("Remove the left hook")) {
                if (sim->alive(leftHook)) sim->world.killEntity(leftHook);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove the right hook")) {
                if (sim->alive(rightHook)) sim->world.killEntity(rightHook);
            }
            ImGui::TextDisabled("Right click a spring to cut it. A removed hook breaks its spring: SpringSystem skips it.");
            // A link between two springs: its fastest mode moves at w = 2 sqrt(k / m), damped as z = c / sqrt(k m).
            const float omega = 2.f * std::sqrt(bridgeStiffness / BridgeMass), zeta = bridgeDamping / std::sqrt(bridgeStiffness * BridgeMass);
            const bool stable = omega * float(Dt) < stabilityLimit(zeta);
            ImGui::TextColored(stable ? Good : Bad, "Stability: dt 2 sqrt(k / m) = %.3f, limit %.3f (z = c / sqrt(k m) = %.2f): %s",
                               double(omega) * Dt, double(stabilityLimit(zeta)), double(zeta), stable ? "stable" : "UNSTABLE");
            if (sim->blownUp > 0) ImGui::TextColored(Bad, "%d links blew up", sim->blownUp);
        }

        void pairScene(ImDrawList* dl) {
            const auto drawTrail = [&](const std::deque<V>& trail, ImU32 color) {
                std::vector<ImVec2> points;
                for (const V& p : trail) points.push_back(view.toScreen(p));
                if (points.size() > 1) dl->AddPolyline(points.data(), int(points.size()), withAlpha(color, 110), ImDrawFlags_None, 1.5f);
            };
            drawTrail(trailA, Color::Series[0]);
            drawTrail(trailB, Color::Series[1]);
            // where the center of mass should be: a straight line at constant speed
            drawDashed(dl, view.toScreen(center0 - centerVelocity * 100.f), view.toScreen(center0 + centerVelocity * 100.f),
                       withAlpha(Color::Marker, 90), 1.f);
            drawTrail(trailCenter, Color::Marker);
        }

        /** The center of mass, over the bodies. */
        void pairCenterMarker(ImDrawList* dl) {
            if (!sim->alive(pairA) || !sim->alive(pairB)) return;
            const ImVec2 c = view.toScreen(pairCenter());
            dl->AddLine({c.x - 8.f, c.y - 8.f}, {c.x + 8.f, c.y + 8.f}, Color::Background, 5.f);
            dl->AddLine({c.x - 8.f, c.y + 8.f}, {c.x + 8.f, c.y - 8.f}, Color::Background, 5.f);
            dl->AddLine({c.x - 7.f, c.y - 7.f}, {c.x + 7.f, c.y + 7.f}, Color::Marker, 2.5f);
            dl->AddLine({c.x - 7.f, c.y + 7.f}, {c.x + 7.f, c.y - 7.f}, Color::Marker, 2.5f);
        }

        void pairPanel() {
            ImGui::SeparatorText("Pair");
            bool changed = ImGui::SliderFloat("m1 (kg)", &pairMass1, 0.2f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("m2 (kg)", &pairMass2, 0.2f, 10.f, "%.2f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("k (N/m)", &pairStiffness, 1.f, 200.f, "%.1f", ImGuiSliderFlags_Logarithmic);
            changed |= ImGui::SliderFloat("c (N s/m)", &pairDamping, 0.f, 10.f, "%.2f");
            if (changed) reset();
            if (!sim->alive(pairA) || !sim->alive(pairB)) {
                ImGui::TextColored(Bad, "A body blew up: lower k.");
                return;
            }
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::Series[0]), "body 1: %.2f kg", double(pairMass1));
            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::Series[1]), "  body 2: %.2f kg", double(pairMass2));
            ImGui::SameLine();
            ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::Marker), "  x center of mass");
            ImGui::TextDisabled("The dashed line is where the center of mass should go: straight on, at constant speed.");
            const Component::Spring& s = sim->world.getComponent<Component::Spring>(pairSpring);
            if (const auto ends = endsOf(*sim, s)) {
                const V f = forceOnBody(s, *ends);
                ImGui::Text("Spring force on body 1 (%+.2f, %+.2f) N, on body 2 the opposite", double(f.x), double(f.y));
            }
            const V p = pairMomentum();
            ImGui::Text("Momentum (%.5f, %.5f) kg m/s", double(p.x), double(p.y));
            ImGui::Text("     at start (%.5f, %.5f): changed by %.2e", double(momentum0.x), double(momentum0.y), double((p - momentum0).length()));
            const V expected = center0 + centerVelocity * float(sim->time - time0);
            ImGui::Text("Center of mass %.2e m from the straight line at %.3f m/s", double((pairCenter() - expected).length()),
                        double(centerVelocity.length()));
            const float energy = pairEnergy();
            ImGui::Text("Energy (kinetic + spring) %.4f J, at start %.4f J", double(energy), double(energy0));
            ImGui::TextDisabled(pairDamping > 0.f ? "c > 0: the damping turns energy into heat; the momentum stays."
                                                  : "c = 0: semi-implicit Euler keeps the energy in a narrow band, without drift.");
            ImGui::TextDisabled("Dragging a body adds an outside force: the momentum changes while you drag, and is measured again "
                                "from the release.");
        }

        // ---------------------------------------------------------------- frame

        void layout(const ImGuiIO& io) {
            sceneSize = {io.DisplaySize.x - PanelWidth, io.DisplaySize.y};
            view.middle = {sceneSize.x * 0.5f, sceneSize.y * 0.5f};
            switch (scene) {
                case Scene::Oscillators: {
                    const float top = 1.2f, bottom = oscEquilibrium() - oscStart - 1.f;
                    view.scale = std::min((sceneSize.x - 40.f) / 13.f, (sceneSize.y - 60.f) / (top - bottom));
                    view.center = V(0.f, (top + bottom) * 0.5f);
                    break;
                }
                case Scene::Rope:
                    view.scale = 40.f;
                    if (sim->alive(ropeBody)) {  // follow the weight, keeping the hook in sight when there is one
                        const V target = roped() ? (sim->position(ropeBody) + sim->world.getComponent<Component::Spring>(ropeSpring).anchor) * 0.5f
                                                 : sim->position(ropeBody);
                        view.center += (target - view.center) * 0.08f;
                    }
                    break;
                case Scene::Bridge:  // room below for the chain hanging from one hook
                    view.scale = std::min((sceneSize.x - 60.f) / 15.f, (sceneSize.y - 40.f) / 14.f);
                    view.center = V(0.f, -5.5f);
                    break;
                case Scene::Pair:
                    view.scale = 45.f;
                    if (sim->alive(pairA) && sim->alive(pairB)) view.center = pairCenter();
                    break;
            }
        }

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
                if (scene == Scene::Rope) {
                    pushDirection = (ImGui::IsKeyDown(ImGuiKey_D) ? 1 : 0) - (ImGui::IsKeyDown(ImGuiKey_A) ? 1 : 0);
                    if (ImGui::IsKeyPressed(ImGuiKey_Space, false) && roped()) {
                        sim->world.killEntity(ropeSpring);
                        ropeSpring = {};
                    }
                }
            }
            handleMouse();
            if (!paused) advance(realSeconds * double(timeScale));
            layout(io);

            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            dl->AddRectFilled({0.f, 0.f}, io.DisplaySize, Color::Background);
            drawGrid(dl, view, sceneSize);
            switch (scene) {
                case Scene::Oscillators: oscillatorsScene(dl); break;
                case Scene::Rope: ropeScene(dl); break;
                case Scene::Bridge: break;
                case Scene::Pair: pairScene(dl); break;
            }
            drawSprings(dl);
            drawBodies(dl);
            if (scene == Scene::Bridge) bridgeScene(dl);  // the arrows go over the hooks
            if (scene == Scene::Pair) pairCenterMarker(dl);

            ImGui::SetNextWindowPos({io.DisplaySize.x - PanelWidth, 0.f});
            ImGui::SetNextWindowSize({PanelWidth, io.DisplaySize.y});
            ImGui::Begin("Spring demo", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::PushTextWrapPos(0.f);

            int sceneIndex = static_cast<int>(scene);
            const char* names[] = {"Oscillators (1)", "Rope (2)", "Bridge (3)", "Pair (4)"};
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
            ImGui::SliderFloat("Time scale", &timeScale, 0.05f, 2.f, "%.2f x");
            ImGui::Text("t = %.3f s, %d steps of 1/60 s", sim->time, sim->steps);

            switch (scene) {
                case Scene::Oscillators: oscillatorsPanel(); break;
                case Scene::Rope: ropePanel(); break;
                case Scene::Bridge: bridgePanel(); break;
                case Scene::Pair: pairPanel(); break;
            }

            ImGui::SeparatorText("About");
            ImGui::TextDisabled("Gravity, springs and integration are the engine's systems. Springs are orange when stretched, "
                                "blue when compressed.");
            ImGui::TextDisabled(scene == Scene::Rope ? "Keys: 1-4 scenes, P pause, R reset. Drag the weight; click elsewhere to hook "
                                                       "the rope there; Space lets go; hold A / D to push."
                                                     : "Keys: 1-4 scenes, P pause, R reset. Drag any body with the left button.");
            ImGui::PopTextWrapPos();
            ImGui::End();
        }
    };

    struct Options {
        const char* screenshot = nullptr;
        Scene scene = Scene::Oscillators;
        double seconds = 0.0;
        float stiffness = -1.f;  // k for Oscillators and Bridge, -1: the default
    };

    Options parse(int argc, char** argv) {
        Options o;
        for (int i = 1; i + 1 < argc; ++i) {
            if (std::strcmp(argv[i], "--screenshot") == 0) {
                o.screenshot = argv[++i];
            } else if (std::strcmp(argv[i], "--scene") == 0) {
                const char* s = argv[++i];
                o.scene = std::strcmp(s, "rope") == 0 ? Scene::Rope : std::strcmp(s, "bridge") == 0 ? Scene::Bridge
                        : std::strcmp(s, "pair") == 0 ? Scene::Pair : Scene::Oscillators;
            } else if (std::strcmp(argv[i], "--seconds") == 0) {
                o.seconds = std::atof(argv[++i]);
            } else if (std::strcmp(argv[i], "--stiffness") == 0) {
                o.stiffness = static_cast<float>(std::atof(argv[++i]));
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
    SDL_Window* window = SDL_CreateWindow("MetroV - spring demo", 1400, 860, SDL_WINDOW_RESIZABLE);
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
    if (options.stiffness > 0.f) {
        demo.oscStiffness = options.stiffness;
        demo.bridgeStiffness = options.stiffness;
    }
    demo.reset();
    if (options.screenshot) {
        demo.paused = true;
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
