// Playground: a platformer character in a level that uses every physics
// feature of the engine, all of it the engine's own (Physics::install).
//
// Zones (keys 1 to 9 and 0, or the panel):
//   1 crates of 5 and 50 kg        2 balls with e = 0.9 / 0.6 / 0.1   3 ice, then rubber
//   4 conveyor belt                5 bounce pad                       6 bridge of springs
//   7 grapple ceiling              8 gravity well and air vent        9 slopes: 45 and 22.5 degrees
//   10 one-way platforms, wall jumps   11 water: floating, sinking, current   12 moving platform
//
// Keys (AZERTY or QWERTY): Q or A / D move, Z, W or Space jump (hold for a full jump),
// S + Space drop through a one-way platform, Shift dash. Right click: grapple (Z or W / S
// reel, Space lets go).
// Left drag: grab a body. E or middle click: explosion at the cursor. Wheel: zoom.
// F1: the engine's debug drawing. P pause, R reset.
//
// The demo's own code: drawing, the controls, a patrolling platform, coins (trigger
// colliders), and the probes that measure jumps and bounces.
//
// Usage: demo_playground [--screenshot file.bmp] [--at zone] [--seconds s] [--debug]
//                        [--move -1|1] [--jump] [--dash] [--grapple x y] [--explode x y]
//        --screenshot runs a scripted scene for s seconds, saves the image and quits:
//        hold A or D, jump or dash after 0.5 s, throw the rope towards (x, y) at the start,
//        an explosion at (x, y) after 0.5 s (world coordinates, in m).

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Physics/DebugDraw.hpp"
#include "Engine/Physics/Physics.hpp"

using namespace Engine;
using V = Math::Vector2f;
using Component::CharacterController;
using Component::Collider;
using Component::PhysicsMaterial;
using Component::RigidBody;
using Component::TileShape;
using Component::Transform;
using System::Phase;

namespace {
    constexpr float G = 9.81f;
    constexpr float PanelWidth = 430.f;
    constexpr float GrappleRange = 18.f;

    namespace Layer {
        constexpr std::uint32_t Default = 1u << 0;
        constexpr std::uint32_t Bridge = 1u << 1;  // the bridge's links do not collide with each other
        constexpr std::uint32_t Pickup = 1u << 2;
    }

    // Materials of the tile map, by index.
    enum Material : std::uint8_t { Stone, Ice, Rubber, Belt, Pad };

    PhysicsMaterial material(float restitution, float staticFriction, float kineticFriction, V surfaceVelocity = {}) {
        PhysicsMaterial m;
        m.restitution = restitution;
        m.staticFriction = staticFriction;
        m.kineticFriction = kineticFriction;
        m.surfaceVelocity = surfaceVelocity;
        return m;
    }

    // ------------------------------------------------------------------ demo-only components and systems

    enum class Style { Crate, Ball, Link, Orb, Platform, Coin, Player };
    struct Look {
        Style style = Style::Crate;
        ImU32 color = 0;
    };

    /** A kinematic platform going back and forth between two x. */
    struct Patrol {
        float min = 0.f;
        float max = 0.f;
        float speed = 2.f;
    };

    class PatrolSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double) override {
                for (auto [entity, transform, body, patrol] : world.view<Transform, RigidBody, Patrol>()) {
                    if (transform.position.x >= patrol.max) body.velocity.x = -patrol.speed;
                    if (transform.position.x <= patrol.min) body.velocity.x = patrol.speed;
                }
            }
            const std::string name() const override { return "PatrolSystem"; }
    };

    /** A ball whose bounces are measured. */
    struct BallProbe {
        float lastVelocity = 0.f;
        float impact = -1.f;  // v out / (v in + g dt) at its last bounce
        int bounces = 0;
    };

    /** What the probes measured, and what the frame must do once the steps are over. */
    struct Measures {
        ECS::Entity player;
        bool wasJumping = false;
        float takeoff = 0.f;
        float apex = 0.f;
        float lastJump = -1.f;
        int coins = 0;
        std::vector<ECS::Entity> collected;  // killed after the steps: no structural change during one
    };

    /** Phase::After: reads the step's results (jumps, bounces, trigger events). */
    class ProbeSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry& world, double dt) override {
                Measures& measures = world.getResource<Measures>();
                const float step = static_cast<float>(dt);
                if (world.hasComponents<CharacterController>(measures.player)) {
                    const CharacterController& c = world.getComponent<CharacterController>(measures.player);
                    const float feet = world.getComponent<Transform>(measures.player).position.y;
                    // The step that jumped has already moved it up by its velocity times dt.
                    const float before = feet - world.getComponent<RigidBody>(measures.player).velocity.y * step;
                    if (c.jumping && !measures.wasJumping) measures.takeoff = measures.apex = before;
                    if (measures.wasJumping || c.jumping) measures.apex = std::max(measures.apex, feet);
                    if (measures.wasJumping && !c.jumping) measures.lastJump = measures.apex - measures.takeoff;
                    measures.wasJumping = c.jumping;
                }
                for (auto [entity, body, probe] : world.view<RigidBody, BallProbe>()) {
                    const float v = body.velocity.y;
                    if (probe.lastVelocity < -1.f && v > 0.f) {  // bounced at this step, on the speed it arrived at
                        probe.impact = v / -probe.lastVelocity;
                        ++probe.bounces;
                    }
                    probe.lastVelocity = v;
                }
                for (const System::TriggerPair& pair : world.getResource<System::TriggerEvents>().entered) {
                    if (pair.other == measures.player && std::find(measures.collected.begin(), measures.collected.end(), pair.trigger) == measures.collected.end()) {
                        measures.collected.push_back(pair.trigger);
                        ++measures.coins;
                    }
                }
            }
            const std::string name() const override { return "ProbeSystem"; }
    };

    // ------------------------------------------------------------------ colors

    namespace Color {
        const ImU32 Background = IM_COL32(24, 26, 33, 255);
        const ImU32 Grid = IM_COL32(32, 35, 43, 255);
        const ImU32 Text = IM_COL32(230, 232, 236, 255);
        const ImU32 Label = IM_COL32(150, 156, 168, 255);
        const ImU32 Tiles[5] = {IM_COL32(84, 82, 78, 255), IM_COL32(150, 205, 235, 255), IM_COL32(130, 62, 62, 255),
                                IM_COL32(62, 64, 74, 255), IM_COL32(90, 200, 130, 255)};
        const ImU32 OneWay = IM_COL32(200, 170, 110, 255);
        const ImU32 Wood = IM_COL32(190, 140, 80, 255);
        const ImU32 Player = IM_COL32(240, 160, 70, 255);
        const ImU32 Visor = IM_COL32(40, 44, 56, 255);
        const ImU32 Link = IM_COL32(170, 120, 70, 255);
        const ImU32 Plank = IM_COL32(120, 85, 50, 255);
        const ImU32 Orb = IM_COL32(150, 110, 230, 255);
        const ImU32 Repulsor = IM_COL32(90, 200, 230, 255);
        const ImU32 Water = IM_COL32(60, 120, 200, 90);
        const ImU32 WaterTop = IM_COL32(120, 180, 240, 200);
        const ImU32 Rope = IM_COL32(225, 215, 190, 255);
        const ImU32 Mouse = IM_COL32(250, 222, 96, 255);
        const ImU32 Coin = IM_COL32(250, 210, 70, 255);
        const ImU32 Platform = IM_COL32(200, 180, 120, 255);
        const ImU32 Blast = IM_COL32(255, 200, 120, 255);
        const ImU32 Ball[3] = {IM_COL32(230, 90, 120, 255), IM_COL32(220, 220, 90, 255), IM_COL32(160, 130, 110, 255)};
    }

    ImU32 withAlpha(ImU32 color, int alpha) { return (color & ~IM_COL32_A_MASK) | (ImU32(std::clamp(alpha, 0, 255)) << IM_COL32_A_SHIFT); }
    ImU32 lighter(ImU32 color, float t) {
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        return ImGui::ColorConvertFloat4ToU32({c.x + (1.f - c.x) * t, c.y + (1.f - c.y) * t, c.z + (1.f - c.z) * t, c.w});
    }
    ImU32 darker(ImU32 color, float t) {
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        return ImGui::ColorConvertFloat4ToU32({c.x * (1.f - t), c.y * (1.f - t), c.z * (1.f - t), c.w});
    }
    const ImVec4 Good{0.5f, 0.9f, 0.55f, 1.f};

    // ------------------------------------------------------------------ the level

    struct Zone {
        const char* name;
        V feet;     // where its key puts the character
        V labelAt;  // where its title is written
        const char* title;
    };

    const std::array<Zone, 12> Zones{{
        {"Crates", {2.f, 0.f}, {2.f, 4.5f}, "1  CRATES of 5 kg and 50 kg: push them"},
        {"Balls", {12.f, 0.f}, {12.5f, 8.f}, "2  BALLS dropped from 6 m: e = 0.9 / 0.6 / 0.1"},
        {"Ice", {21.f, 0.f}, {21.f, 4.5f}, "3  ICE, then RUBBER: slide, then grip"},
        {"Belt", {37.f, 0.f}, {36.5f, 4.5f}, "4  CONVEYOR BELT at 3 m/s"},
        {"Pad", {45.8f, 4.f}, {44.f, 7.5f}, "5  BOUNCE PAD e = 0.9: walk off the ledge"},
        {"Bridge", {53.5f, 0.f}, {54.5f, 4.5f}, "6  BRIDGE of springs: it bends under you"},
        {"Grapple", {69.f, 0.f}, {68.f, 7.5f}, "7  GRAPPLE: right click the ceiling, Z or W / S reel"},
        {"Well", {81.f, 0.f}, {80.5f, 8.f}, "8  GRAVITY WELL: walk under it | AIR VENT: a radial field from below"},
        {"Slopes", {96.5f, 0.f}, {96.5f, 5.f}, "9  SLOPES: 45 then 22.5 degrees"},
        {"Walls", {118.5f, 0.f}, {118.5f, 13.f}, "10  ONE-WAY platforms (S + Space drops), WALL JUMPS up the shaft"},
        {"Water", {133.f, 0.f}, {133.f, 4.f}, "11  WATER: crates float or sink, a current; jump to swim"},
        {"Platform", {147.f, 0.f}, {148.f, 4.f}, "12  MOVING PLATFORM: ride it across"},
    }};

    struct Sim {
        ECS::Registry world;
        ECS::SystemManager systems;
        System::PhysicsSystem* physics = nullptr;
        ECS::Entity level;
        ECS::Entity player;
        std::vector<ECS::Entity> balls;
    };

    /** Fills the cells covering world [x0, x1) x [y0, y1) of a map whose origin is on the grid. */
    void fill(Component::TileMap& map, int x0, int x1, int y0, int y1, TileShape shape, std::uint8_t mat = Stone) {
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) map.set(x - static_cast<int>(map.origin.x), y - static_cast<int>(map.origin.y), {shape, mat});
        }
    }
    void put(Component::TileMap& map, int x, int y, TileShape shape) { fill(map, x, x + 1, y, y + 1, shape); }

    Component::TileMap buildLevel() {
        Component::TileMap map;
        map.origin = {-2.f, -10.f};
        map.resize(164, 26);
        map.materials = {material(0.f, 0.6f, 0.5f), material(0.f, 0.1f, 0.05f), material(0.f, 1.f, 0.9f),
                         material(0.f, 0.8f, 0.7f, {3.f, 0.f}), material(0.9f, 0.6f, 0.5f)};
        fill(map, -2, -1, -10, 15, TileShape::Solid);    // left wall
        fill(map, 160, 162, -10, 15, TileShape::Solid);  // right wall
        fill(map, -1, 20, -10, 0, TileShape::Solid);
        fill(map, 20, 30, -10, 0, TileShape::Solid, Ice);
        fill(map, 30, 36, -10, 0, TileShape::Solid, Rubber);
        fill(map, 36, 44, -10, 0, TileShape::Solid, Belt);
        fill(map, 44, 47, -10, 0, TileShape::Solid);
        fill(map, 44, 45, 0, 2, TileShape::Solid);    // a step up to the ledge
        fill(map, 45, 47, 3, 4, TileShape::OneWay);   // the ledge: jump onto it from below
        fill(map, 47, 53, -10, 0, TileShape::Solid, Pad);
        fill(map, 53, 54, -10, 0, TileShape::Solid);
        fill(map, 54, 67, -10, -8, TileShape::Solid);  // the bridge's pit
        fill(map, 67, 71, -10, 0, TileShape::Solid);
        fill(map, 71, 79, -10, -8, TileShape::Solid);  // the grapple's pit
        fill(map, 79, 96, -10, 0, TileShape::Solid);
        fill(map, 66, 84, 9, 10, TileShape::Solid);    // the grapple's ceiling
        // 9: a 45 degree hill, then a 22.5 degree one.
        fill(map, 96, 118, -10, 0, TileShape::Solid);
        put(map, 98, 0, TileShape::SlopeUp);
        fill(map, 99, 104, 0, 1, TileShape::Solid);
        put(map, 99, 1, TileShape::SlopeUp);
        fill(map, 100, 103, 1, 2, TileShape::Solid);
        put(map, 103, 1, TileShape::SlopeDown);
        put(map, 104, 0, TileShape::SlopeDown);
        put(map, 107, 0, TileShape::GentleUpLow);
        put(map, 108, 0, TileShape::GentleUpHigh);
        fill(map, 109, 115, 0, 1, TileShape::Solid);
        put(map, 109, 1, TileShape::GentleUpLow);
        put(map, 110, 1, TileShape::GentleUpHigh);
        fill(map, 111, 113, 1, 2, TileShape::Solid);
        put(map, 113, 1, TileShape::GentleDownHigh);
        put(map, 114, 1, TileShape::GentleDownLow);
        put(map, 115, 0, TileShape::GentleDownHigh);
        put(map, 116, 0, TileShape::GentleDownLow);
        // 10: one-way platforms, and a shaft to climb by wall jumps.
        fill(map, 118, 132, -10, 0, TileShape::Solid);
        fill(map, 119, 122, 2, 3, TileShape::OneWay);
        fill(map, 122, 125, 5, 6, TileShape::OneWay);
        fill(map, 119, 122, 8, 9, TileShape::OneWay);
        fill(map, 126, 127, 0, 13, TileShape::Solid);
        fill(map, 129, 130, 0, 10, TileShape::Solid);
        fill(map, 130, 132, 10, 11, TileShape::Solid);
        // 11: a pool.
        fill(map, 132, 134, -10, 0, TileShape::Solid);
        fill(map, 134, 146, -10, -5, TileShape::Solid);
        fill(map, 146, 148, -10, 0, TileShape::Solid);
        // 12: a pit to cross on the moving platform.
        fill(map, 148, 158, -10, -8, TileShape::Solid);
        fill(map, 158, 160, -10, 0, TileShape::Solid);
        return map;
    }
}

namespace {
    struct View {
        V center;
        float scale = 40.f;  // pixels per metre
        ImVec2 middle;
        [[nodiscard]] ImVec2 toScreen(V w) const { return {middle.x + (w.x - center.x) * scale, middle.y - (w.y - center.y) * scale}; }
        [[nodiscard]] V toWorld(ImVec2 s) const { return {center.x + (s.x - middle.x) / scale, center.y - (s.y - middle.y) / scale}; }
    };

    void drawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color, float thickness = 2.f) {
        dl->AddLine(from, to, color, thickness);
        const float dx = to.x - from.x, dy = to.y - from.y, length = std::sqrt(dx * dx + dy * dy);
        if (length < 4.f) return;
        const float ux = dx / length, uy = dy / length, head = std::min(9.f, length * 0.5f);
        dl->AddTriangleFilled(to, {to.x - ux * head - uy * head * 0.5f, to.y - uy * head + ux * head * 0.5f},
                              {to.x - ux * head + uy * head * 0.5f, to.y - uy * head - ux * head * 0.5f}, color);
    }

    void drawDashed(ImDrawList* dl, ImVec2 a, ImVec2 b, ImU32 color) {
        const float dx = b.x - a.x, dy = b.y - a.y, length = std::sqrt(dx * dx + dy * dy);
        for (float s = 0.f; s < length; s += 12.f) {
            const float e = std::min(s + 7.f, length);
            dl->AddLine({a.x + dx * s / length, a.y + dy * s / length}, {a.x + dx * e / length, a.y + dy * e / length}, color, 1.5f);
        }
    }

    struct PlankSpring {};  // drawn as a plank
    struct Blast {
        V center;
        float radius;
        double time;
    };

    struct Demo {
        std::unique_ptr<Sim> sim;
        View view;
        ImVec2 sceneSize;
        bool paused = false;
        bool debug = false;
        bool snapCamera = true;
        float timeScale = 1.f;
        int zone = 0;
        struct {
            bool gravity = true, springs = true, fields = true, fluids = true, friction = true, ccd = true, response = true;
        } enabled;
        CharacterController tuning;  // the character's tuning, kept across resets
        ECS::Entity rope;            // the grapple's spring, or none
        ECS::Entity drag;            // the mouse's spring, or none
        ECS::Entity dragged;
        float explosionSpeed = 12.f, explosionRadius = 5.f;
        std::vector<Blast> blasts;
        double physicsMilliseconds = 0.0;
        double time = 0.0;

        Demo() {
            tuning.jumpHeight = 2.6f;
        }

        ECS::Registry& world() { return sim->world; }
        CharacterController& controller() { return world().getComponent<CharacterController>(sim->player); }
        RigidBody& rigid(ECS::Entity e) { return world().getComponent<RigidBody>(e); }
        V position(ECS::Entity e) { return world().getComponent<Transform>(e).position; }
        bool alive(ECS::Entity e) { return world().hasComponents<Transform>(e); }
        bool roped() { return world().hasComponents<Component::Spring>(rope); }

        // ---------------------------------------------------------------- setup

        void reset() {
            rope = {};
            drag = {};
            blasts.clear();
            time = 0.0;
            sim = std::make_unique<Sim>();
            ECS::Registry& w = world();
            w.setResource<Resource::GravitySettings>(Resource::GravitySettings{V(0.f, -G), Physic::GravitationalConstant<float>});
            w.setResource<Measures>();
            setupPhysics();
            sim->level = w.spawnEntity("level");
            w.addComponent(sim->level, buildLevel());
            buildBridge();
            settleBridge();
            spawnBodies();
            teleport(zone);
        }

        /** The engine's systems in their phases, without the ones switched off in the panel. */
        void setupPhysics() {
            ECS::Registry& w = world();
            if (sim->physics) sim->systems.removeSystem<System::PhysicsSystem>(w);
            System::PhysicsSystem& physics = sim->systems.addSystem<System::PhysicsSystem>(w);
            physics.add<PatrolSystem>(Phase::Control, w);
            physics.add<System::CharacterControllerSystem>(Phase::Control, w);
            if (enabled.gravity) physics.add<System::GravitySystem>(Phase::Forces, w);
            if (enabled.springs) physics.add<System::SpringSystem>(Phase::Forces, w);
            if (enabled.fields) physics.add<System::RadialFieldSystem>(Phase::Forces, w);
            if (enabled.fluids) {
                physics.add<System::DragSystem>(Phase::Forces, w);
                physics.add<System::FluidSystem>(Phase::Forces, w);
            }
            if (enabled.friction) physics.add<System::FrictionSystem>(Phase::Constraints, w);
            physics.add<System::IntegrationSystem>(Phase::Integrate, w);
            if (enabled.ccd) physics.add<System::ContinuousCollisionSystem>(Phase::Integrate, w);
            physics.add<System::CollisionSystem>(Phase::Detect, w);
            if (enabled.response) {
                physics.add<System::CollisionResponseSystem>(Phase::Respond, w);
            } else {
                for (auto [entity, surface] : w.view<Component::SurfaceContact>()) surface.touching = false;
            }
            physics.add<ProbeSystem>(Phase::After, w);
            sim->physics = &physics;
        }

        ECS::Entity body(V center, float mass, Collider collider, Look look, const char* name, V velocity = {}) {
            ECS::Registry& w = world();
            const ECS::Entity e = w.spawnEntity(name);
            w.addComponent(e, Transform{center});
            RigidBody rigidBody;
            rigidBody.mass = mass;
            rigidBody.velocity = velocity;
            w.addComponent(e, rigidBody);
            w.addComponent(e, collider);
            w.addComponent(e, look);
            return e;
        }

        ECS::Entity crate(V center, float half, float mass, ImU32 color = Color::Wood) {
            Collider c;
            c.halfSize = {half, half};
            c.material = material(0.1f, 0.6f, 0.5f);
            return body(center, mass, c, Look{Style::Crate, color}, "crate");
        }

        ECS::Entity ball(V center, float radius, float mass, float restitution, ImU32 color) {
            Collider c;
            c.shape = Component::ColliderShape::Circle;
            c.radius = radius;
            c.material = material(restitution, 0.6f, 0.5f);
            return body(center, mass, c, Look{Style::Ball, color}, "ball");
        }

        ECS::Entity coin(V center) {
            ECS::Registry& w = world();
            const ECS::Entity e = w.spawnEntity("coin");
            w.addComponent(e, Transform{center});
            Collider c;
            c.shape = Component::ColliderShape::Circle;
            c.radius = 0.35f;
            c.trigger = true;
            c.layer = Layer::Pickup;
            w.addComponent(e, c);
            w.addComponent(e, Look{Style::Coin, Color::Coin});
            return e;
        }

        ECS::Entity spring(ECS::Entity end, ECS::Entity other, V anchor, float rest, float stiffness, float damping, bool asRope = false) {
            Component::Spring s;
            s.body = end;
            s.other = other;
            s.anchor = anchor;
            s.restLength = rest;
            s.stiffness = stiffness;
            s.damping = damping;
            s.rope = asRope;
            const ECS::Entity e = world().spawnEntity("spring");
            world().addComponent(e, s);
            return e;
        }

        void buildBridge() {
            // A link between two springs: stable while dt 2 sqrt(k / m) < 2 (sqrt(1 + z^2) - z), z = c / sqrt(k m): 1.05 < 1.58.
            ECS::Registry& w = world();
            const V left(54.f, -0.3f), right(67.f, -0.3f);
            const auto hook = [&](V at) {
                const ECS::Entity e = w.spawnEntity("hook");
                w.addComponent(e, Transform{at});
                return e;
            };
            ECS::Entity previous = hook(left);
            const ECS::Entity end = hook(right);
            for (int i = 1; i <= 20; ++i) {
                Collider c;
                c.shape = Component::ColliderShape::Circle;
                c.radius = 0.27f;
                c.material = material(0.f, 0.8f, 0.7f);
                c.layer = Layer::Bridge;
                c.mask = ~Layer::Bridge;
                const ECS::Entity link = body(left + (right - left) * (float(i) / 21.f), 8.f, c, Look{Style::Link, Color::Link}, "link");
                w.addComponent(spring(link, previous, {}, 0.58f, 8000.f, 60.f), PlankSpring{});
                previous = link;
            }
            w.addComponent(spring(end, previous, {}, 0.58f, 8000.f, 60.f), PlankSpring{});
        }

        /** The bridge starts at rest in the shape it hangs in: settled first with extra damping (demo-only). */
        void settleBridge() {
            for (int i = 0; i < 900; ++i) {
                sim->physics->step(world());
                for (auto [e, b] : world().view<RigidBody>()) b.velocity = b.velocity * 0.9f;
            }
            for (auto [e, b] : world().view<RigidBody>()) b.velocity = V::zero();
        }

        void spawnBodies() {
            ECS::Registry& w = world();
            // The character: kinematic (mass 0), moved by its controller.
            sim->player = w.spawnEntity("player");
            w.addComponent(sim->player, Transform{Zones[0].feet + V(0.f, 0.9f + tuning.skin)});
            Collider hero;
            hero.halfSize = {0.4f, 0.9f};
            hero.material = material(0.f, 1.f, 1.f);
            w.addComponent(sim->player, hero);
            RigidBody kinematic;
            kinematic.mass = 0.f;
            w.addComponent(sim->player, kinematic);
            w.addComponent(sim->player, tuning);
            w.addComponent(sim->player, Look{Style::Player, Color::Player});
            w.getResource<Measures>().player = sim->player;

            crate({6.f, 0.3f}, 0.3f, 5.f);  // 1
            crate({9.f, 0.6f}, 0.6f, 50.f);
            const float bounciness[] = {0.9f, 0.6f, 0.1f};
            sim->balls.clear();
            for (int i = 0; i < 3; ++i) {  // 2
                const ECS::Entity b = ball({14.f + 2.f * float(i), 6.3f}, 0.3f, 0.5f, bounciness[i], Color::Ball[i]);
                w.addComponent(b, BallProbe{});
                sim->balls.push_back(b);
            }
            crate({25.f, 0.4f}, 0.4f, 10.f);  // 3, on the ice
            crate({38.f, 0.4f}, 0.4f, 10.f);  // 4, on the belt
            ball({50.f, 3.3f}, 0.3f, 0.5f, 0.5f, Color::Ball[1]);  // 5, onto the pad

            // 8: a gravity well (a static orb with a GravityAttractor, 4.5 m of range): G M = 167, so it
            // pulls 14 m/s² at 3.5 m, more than gravity: what passes under it is lifted to it.
            const ECS::Entity well = w.spawnEntity("gravity well");
            w.addComponent(well, Transform{{85.5f, 3.8f}});
            Collider orb;
            orb.shape = Component::ColliderShape::Circle;
            orb.radius = 0.8f;
            orb.material = material(0.2f, 0.6f, 0.5f);
            w.addComponent(well, orb);
            w.addComponent(well, Component::GravityAttractor{2.5e12f, 4.5f});
            w.addComponent(well, Look{Style::Orb, Color::Orb});
            // An air vent: a RadialField centered 1 m under the ground pushes up what is above it. Its push
            // equals gravity 3 m from its center: bodies float about 2 m above the ground. Out of the well's range.
            const ECS::Entity vent = w.spawnEntity("air vent");
            w.addComponent(vent, Transform{{94.f, -1.f}});
            w.addComponent(vent, Component::RadialField{30.f, 4.5f, true});
            ball({82.5f, 0.3f}, 0.3f, 0.5f, 0.6f, Color::Ball[1]);
            ball({85.5f, 0.3f}, 0.3f, 0.5f, 0.2f, Color::Ball[2]);  // right under the well
            ball({93.6f, 3.f}, 0.3f, 0.5f, 0.9f, Color::Ball[0]);    // over the vent
            crate({94.4f, 3.f}, 0.3f, 5.f);

            crate({101.f, 2.4f}, 0.4f, 10.f);                       // 9, on the 45 degree hill
            ball({111.5f, 2.3f}, 0.3f, 1.f, 0.2f, Color::Ball[2]);  // on the gentle one

            // 11: water. Light crates float, the 60 kg one (60 kg/m²) sinks; the current carries them.
            const ECS::Entity water = w.spawnEntity("water");
            w.addComponent(water, Transform{{140.f, -2.75f}});
            Component::FluidVolume fluid;
            fluid.halfSize = {6.f, 2.25f};
            fluid.density = 55.f;  // the character (49 kg/m²) floats, 88 % under; the 60 kg crate sinks
            fluid.drag = 80.f;     // thick enough to swim in
            fluid.flow = {1.f, 0.f};
            w.addComponent(water, fluid);
            crate({136.f, 0.5f}, 0.4f, 10.f);
            crate({139.f, 0.5f}, 0.5f, 20.f);
            crate({142.f, 0.5f}, 0.5f, 60.f, darker(Color::Wood, 0.35f));  // 60 kg/m²: sinks
            ball({144.f, 0.5f}, 0.4f, 4.f, 0.1f, Color::Ball[2]);

            // 12: a kinematic platform going back and forth.
            Collider deck;
            deck.halfSize = {1.5f, 0.25f};
            const ECS::Entity platform = body({151.f, -0.25f}, 0.f, deck, Look{Style::Platform, Color::Platform}, "platform", {2.f, 0.f});
            w.addComponent(platform, Patrol{150.5f, 156.5f, 2.f});

            for (const V at : {V(46.f, 5.4f), V(60.5f, 1.6f), V(75.f, 5.f), V(101.5f, 3.6f), V(123.5f, 6.9f), V(131.f, 11.9f),
                               V(140.f, -1.2f), V(153.f, 2.f)}) {
                coin(at);
            }
        }

        // ---------------------------------------------------------------- actions

        void teleport(int z) {
            zone = z;
            release();
            if (!alive(sim->player)) return;
            world().getComponent<Transform>(sim->player).position = Zones[static_cast<std::size_t>(z)].feet + V(0.f, 0.9f + tuning.skin);
            controller().velocity = V::zero();
            snapCamera = true;
        }

        /** Throws the rope: the first thing the ray hits (Physics::raycast) holds it. The character is handed to physics. */
        void grapple(V toward) {
            const V from = position(sim->player);
            const V direction = toward - from;
            if (direction.length() < 0.5f) return;
            Physics::QueryFilter filter;
            filter.ignore = sim->player;
            filter.mask = ~Layer::Pickup;
            const auto hit = Physics::raycast(world(), from, direction * (GrappleRange / direction.length()), filter);
            if (!hit) return;
            const bool toBody = !hit->tile && world().hasComponents<RigidBody>(hit->entity) && rigid(hit->entity).mass > 0.f;
            constexpr float Mass = 70.f;
            const float reduced = toBody ? Mass * rigid(hit->entity).mass / (Mass + rigid(hit->entity).mass) : Mass;
            // A rope 1 m shorter than the distance: taut at once, it lifts the character off the
            // ground (softly: k = 150 m, critically damped) and swings it.
            const float k = 150.f * reduced;
            const float length = ((toBody ? position(hit->entity) : hit->hit.point) - from).length();
            rope = spring(sim->player, toBody ? hit->entity : ECS::Entity{}, hit->hit.point, std::max(1.f, length - 1.f), k,
                          Physic::criticalDamping(k, reduced), true);
            updateRope();
        }

        [[nodiscard]] float ropeDistance() {
            const Component::Spring& s = world().getComponent<Component::Spring>(rope);
            const V hook = s.other == ECS::Entity{} || !alive(s.other) ? s.anchor : position(s.other);
            return (position(sim->player) - hook).length();
        }

        /**
         * On the rope, the character is handed to physics when it is off the ground or the rope
         * pulls (a body of 70 kg that the rope swings), and taken back by its controller when it
         * stands with the rope slack, so that it walks while hooked. A kinematic body would not
         * feel the rope's pull.
         */
        void updateRope() {
            if (!roped()) return;
            CharacterController& c = controller();
            RigidBody& body = rigid(sim->player);
            const float slack = world().getComponent<Component::Spring>(rope).restLength - ropeDistance();  // > 0: slack
            if (c.enabled && (!c.grounded || slack < -0.05f)) {  // off the ground, or the rope pulls
                body.mass = 70.f;
                body.velocity = c.velocity + c.groundVelocity;
                c.enabled = false;
            } else if (!c.enabled && slack > 0.05f && world().hasComponents<Component::SurfaceContact>(sim->player)) {
                const Component::SurfaceContact& surface = world().getComponent<Component::SurfaceContact>(sim->player);
                if (surface.touching && surface.normal.y > 0.7f && body.velocity.y <= 0.1f) {
                    c.velocity = body.velocity;
                    c.enabled = true;
                    c.grounded = true;
                    body.mass = 0.f;
                }
            }
        }

        /** Lets go of the rope: the character takes over again. */
        void release() {
            if (roped()) world().killEntity(rope);
            rope = {};
            if (!alive(sim->player) || controller().enabled) return;
            controller().velocity = rigid(sim->player).velocity;
            controller().enabled = true;
            controller().jumping = false;
            rigid(sim->player).mass = 0.f;
        }

        void explode(V center) {
            Physics::applyRadialImpulse(world(), center, explosionSpeed, explosionRadius);  // bodies and the character
            blasts.push_back({center, explosionRadius, time});
        }

        std::optional<ECS::Entity> bodyAt(V point) {
            std::optional<ECS::Entity> best;
            float bestDistance = 1e9f;
            for (auto [e, transform, body, collider] : world().view<Transform, RigidBody, Collider>()) {
                if (body.mass <= 0.f) continue;
                const float reach = collider.shape == Component::ColliderShape::Box ? std::max(collider.halfSize.x, collider.halfSize.y) : collider.radius;
                const float d = (transform.position - point).length();
                if (d < reach + 0.2f && d < bestDistance) {
                    best = e;
                    bestDistance = d;
                }
            }
            return best;
        }

        // ---------------------------------------------------------------- input

        void input(const ImGuiIO& io) {
            const V mouse = view.toWorld(io.MousePos);
            const bool inScene = !io.WantCaptureMouse && io.MousePos.x < sceneSize.x;
            CharacterController& c = controller();
            if (!io.WantCaptureKeyboard) {
                // Letters as typed (ImGui follows the keyboard layout): Z Q S D on AZERTY, W A S D on QWERTY.
                const bool left = ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_LeftArrow);
                const bool right = ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow);
                c.move = (right ? 1.f : 0.f) - (left ? 1.f : 0.f);
                const bool up = ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_Z) || ImGui::IsKeyDown(ImGuiKey_UpArrow);
                const bool down = ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow);
                const bool jumpKey = ImGui::IsKeyPressed(ImGuiKey_Space, false) || ImGui::IsKeyPressed(ImGuiKey_W, false) ||
                                     ImGui::IsKeyPressed(ImGuiKey_Z, false) || ImGui::IsKeyPressed(ImGuiKey_UpArrow, false);
                if (roped()) {
                    Component::Spring& s = world().getComponent<Component::Spring>(rope);
                    const float reel = (up ? 1.f : 0.f) - (down ? 1.f : 0.f);
                    s.restLength = std::clamp(s.restLength - reel * 4.f * io.DeltaTime, 1.f, 25.f);
                    if (!c.enabled) rigid(sim->player).force.x += c.move * 250.f;  // swing
                    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) release();
                } else {
                    if (jumpKey && down) {
                        c.dropPressed = true;
                    } else if (jumpKey) {
                        c.jumpPressed = true;
                    }
                    c.jumpHeld = ImGui::IsKeyDown(ImGuiKey_Space) || up;
                    if (ImGui::IsKeyPressed(ImGuiKey_LeftShift, false) || ImGui::IsKeyPressed(ImGuiKey_RightShift, false)) c.dashPressed = true;
                }
                for (int i = 0; i < 9; ++i) {  // 1 to 9 (ImGuiKey_9 + 1 is ImGuiKey_A: the A key used to teleport)
                    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i), false)) teleport(i);
                }
                if (ImGui::IsKeyPressed(ImGuiKey_0, false)) teleport(9);
                if (ImGui::IsKeyPressed(ImGuiKey_R, false)) reset();
                if (ImGui::IsKeyPressed(ImGuiKey_P, false)) paused = !paused;
                if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) debug = !debug;
                if (ImGui::IsKeyPressed(ImGuiKey_E, false) && inScene) explode(mouse);
            }
            if (world().hasComponents<Component::Spring>(drag)) {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !alive(dragged)) {
                    world().killEntity(drag);
                    drag = {};
                } else {
                    world().getComponent<Component::Spring>(drag).anchor = mouse;
                }
            }
            if (!inScene) return;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (const auto target = bodyAt(mouse)) {
                    const float mass = rigid(*target).mass, k = 60.f * mass;
                    dragged = *target;
                    drag = spring(*target, {}, mouse, 0.f, k, Physic::criticalDamping(k, mass));
                }
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (roped()) {
                    release();
                } else {
                    grapple(mouse);
                }
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) explode(mouse);
            if (io.MouseWheel != 0.f) view.scale = std::clamp(view.scale * std::pow(1.15f, io.MouseWheel), 12.f, 120.f);
        }

        /** After the frame's steps: what had to wait for them (structural changes), and what fell out. */
        void afterSteps() {
            Measures& measures = world().getResource<Measures>();
            for (const ECS::Entity coin : measures.collected) {
                if (alive(coin)) world().killEntity(coin);
            }
            measures.collected.clear();
            std::vector<ECS::Entity> fallen;
            for (auto [e, transform, body] : world().view<Transform, RigidBody>()) {
                if (transform.position.y < -40.f || !std::isfinite(transform.position.x) || !std::isfinite(transform.position.y)) fallen.push_back(e);
            }
            for (const ECS::Entity e : fallen) {
                if (e == sim->player) {
                    teleport(zone);
                } else {
                    world().killEntity(e);
                }
            }
        }

        // ---------------------------------------------------------------- drawing

        void drawLevel(ImDrawList* dl) {
            const Component::TileMap& map = world().getComponent<Component::TileMap>(sim->level);
            const Physics::CellRange cells = Physics::cellsUnder(map, Collision::Rectangle{view.toWorld({0.f, sceneSize.y}), view.toWorld({sceneSize.x, 0.f})});
            for (int y = cells.y0; y <= cells.y1; ++y) {
                for (int x = cells.x0; x <= cells.x1; ++x) {
                    const Component::Tile tile = map.at(x, y);
                    if (tile.shape == TileShape::Empty) continue;
                    if (tile.shape == TileShape::OneWay) {
                        const Collision::Rectangle cell = Physics::cellBounds(map, x, y);
                        dl->AddRectFilled(view.toScreen({cell.min.x, cell.max.y}), view.toScreen({cell.max.x, cell.max.y - 0.25f}), Color::OneWay);
                        continue;
                    }
                    const Collision::Polygon polygon = Physics::tilePolygon(map, x, y);
                    std::array<ImVec2, Collision::Polygon::MaxVertices> points{};
                    for (int i = 0; i < polygon.count; ++i) points[static_cast<std::size_t>(i)] = view.toScreen(polygon.vertex(i));
                    const ImU32 color = Color::Tiles[std::min<std::size_t>(tile.material, 4)];
                    dl->AddConvexPolyFilled(points.data(), polygon.count, color);
                    const std::array<bool, 4> exposed = Physics::exposedSides(map, x, y);
                    for (int i = 0; i < polygon.count; ++i) {  // the surface: exposed edges facing up
                        if (polygon.normal(i).y > 0.3f && Physics::pushAllowed(exposed, polygon.normal(i))) {
                            dl->AddLine(points[static_cast<std::size_t>(i)], points[static_cast<std::size_t>(polygon.next(i))], lighter(color, 0.35f), 3.f);
                        }
                    }
                    const float belt = map.materialOf(tile).surfaceVelocity.x;
                    if (belt != 0.f && exposed[3]) {  // moving stripes on a belt's top
                        const Collision::Rectangle cell = Physics::cellBounds(map, x, y);
                        const float s = cell.min.x + std::fmod(static_cast<float>(time) * belt, 1.f);
                        dl->AddLine(view.toScreen({s, cell.max.y - 0.1f}), view.toScreen({std::min(s + 0.3f, cell.max.x), cell.max.y - 0.6f}), lighter(color, 0.2f), 2.f);
                    }
                }
            }
        }

        void drawWorld(ImDrawList* dl, const ImGuiIO& io) {
            ECS::Registry& w = world();
            dl->AddRectFilled({0.f, 0.f}, io.DisplaySize, Color::Background);
            const V low = view.toWorld({0.f, sceneSize.y}), high = view.toWorld({sceneSize.x, 0.f});
            for (float x = std::floor(low.x); x <= high.x; x += 1.f) dl->AddLine(view.toScreen({x, low.y}), view.toScreen({x, high.y}), Color::Grid);
            for (float y = std::floor(low.y); y <= high.y; y += 1.f) dl->AddLine(view.toScreen({low.x, y}), view.toScreen({high.x, y}), Color::Grid);
            for (const Zone& z : Zones) dl->AddText(view.toScreen(z.labelAt), Color::Label, z.title);

            for (auto [e, transform, field] : w.view<Transform, Component::RadialField>()) {
                const ImVec2 c = view.toScreen(transform.position);
                for (int i = 0; i < 3; ++i) {
                    const float r = std::fmod(static_cast<float>(time) * 0.8f + float(i) / 3.f, 1.f);
                    dl->AddCircle(c, r * field.radius * view.scale, withAlpha(Color::Repulsor, int(170.f * (1.f - r))), 64, 2.f);
                }
                dl->AddCircleFilled(c, 0.25f * view.scale, Color::Repulsor);
            }
            drawLevel(dl);
            for (auto [e, transform, fluid] : w.view<Transform, Component::FluidVolume>()) {
                const Collision::Rectangle r = Collision::Rectangle::fromCenter(transform.position + fluid.offset, fluid.halfSize);
                dl->AddRectFilled(view.toScreen({r.min.x, r.max.y}), view.toScreen({r.max.x, r.min.y}), Color::Water);
                dl->AddLine(view.toScreen({r.min.x, r.max.y}), view.toScreen(r.max), Color::WaterTop, 2.f);
            }

            for (auto [e, spring] : w.view<Component::Spring>()) {
                const bool toPoint = spring.other == ECS::Entity{};
                if (!alive(spring.body) || (!toPoint && !alive(spring.other))) continue;
                const ImVec2 a = view.toScreen(position(spring.body)), b = view.toScreen(toPoint ? spring.anchor : position(spring.other));
                if (w.hasComponents<PlankSpring>(e)) {
                    dl->AddLine(a, b, Color::Plank, std::max(2.f, 0.18f * view.scale));
                } else if (e == drag) {
                    dl->AddLine(a, b, Color::Mouse, 1.5f);
                    dl->AddCircleFilled(b, 4.f, Color::Mouse);
                } else {
                    dl->AddLine(a, b, Color::Rope, 2.f);
                    if (toPoint) dl->AddCircle(b, 5.f, Color::Rope, 0, 2.f);
                }
            }

            for (auto [e, transform, collider, look] : w.view<Transform, Collider, Look>()) {
                const Physics::Shape shape = Physics::shapeOf(transform, collider);
                if (const auto* circle = std::get_if<Collision::Circle>(&shape)) {
                    const ImVec2 c = view.toScreen(circle->center);
                    const float r = std::max(3.f, circle->radius * view.scale);
                    if (look.style == Style::Orb) {
                        for (int i = 4; i >= 1; --i) dl->AddCircleFilled(c, r * (1.f + 0.5f * float(i)), withAlpha(Color::Orb, 18));
                    }
                    if (look.style == Style::Coin) {
                        const float squash = std::abs(std::cos(static_cast<float>(time) * 3.f));
                        dl->AddEllipseFilled(c, {std::max(2.f, r * squash), r}, look.color);
                        continue;
                    }
                    dl->AddCircleFilled(c, r, look.color);
                    dl->AddCircle(c, r, darker(look.color, 0.45f), 0, 1.5f);
                    continue;
                }
                const Collision::Rectangle box = std::get<Collision::Rectangle>(shape);
                const ImVec2 a = view.toScreen({box.min.x, box.max.y}), b = view.toScreen({box.max.x, box.min.y});
                const float rounding = look.style == Style::Player ? 4.f : 0.f;
                dl->AddRectFilled(a, b, look.color, rounding);
                dl->AddRect(a, b, darker(look.color, 0.45f), rounding, 0, 1.5f);
                if (look.style == Style::Crate) {
                    dl->AddLine(a, b, darker(look.color, 0.3f), 2.f);
                    dl->AddLine({a.x, b.y}, {b.x, a.y}, darker(look.color, 0.3f), 2.f);
                } else if (look.style == Style::Player) {
                    const float facing = static_cast<float>(controller().facing);
                    const float x0 = transform.position.x + facing * 0.12f - 0.25f, top = box.max.y;
                    dl->AddRectFilled(view.toScreen({x0, top - 0.25f}), view.toScreen({x0 + 0.5f, top - 0.5f}), Color::Visor, 3.f);
                    dl->AddCircleFilled(view.toScreen({x0 + 0.25f + facing * 0.14f, top - 0.37f}), std::max(1.5f, 0.05f * view.scale), IM_COL32(120, 230, 255, 255));
                    if (controller().dashing) dl->AddRect(a, b, IM_COL32(255, 255, 255, 180), rounding, 0, 3.f);
                }
            }

            if (debug) {  // the engine's own debug drawing
                const Physics::DebugDrawing drawing = Physics::debugDraw(w);
                for (const Physics::DebugLine& line : drawing.lines) {
                    if (line.arrow) {
                        drawArrow(dl, view.toScreen(line.from), view.toScreen(line.to), line.color, 1.5f);
                    } else {
                        dl->AddLine(view.toScreen(line.from), view.toScreen(line.to), line.color, 1.5f);
                    }
                }
                for (const Physics::DebugCircle& circle : drawing.circles) dl->AddCircle(view.toScreen(circle.center), circle.radius * view.scale, circle.color, 0, 1.5f);
                dl->AddText({12.f, 10.f}, Color::Text, "Physics::debugDraw: colliders (blue dynamic, yellow kinematic, gray static, green triggers), level outline, contacts, velocities");
            }

            for (const Blast& blast : blasts) {
                const float t = static_cast<float>(time - blast.time) / 0.35f;
                if (t <= 1.f) dl->AddCircle(view.toScreen(blast.center), blast.radius * view.scale * t, withAlpha(Color::Blast, int(255.f * (1.f - t))), 64, 3.f);
            }
            std::erase_if(blasts, [&](const Blast& blast) { return time - blast.time > 0.35; });

            const bool inScene = !io.WantCaptureMouse && io.MousePos.x < sceneSize.x;
            if (inScene && !roped()) {  // where a rope thrown now would hold
                const V from = position(sim->player), to = view.toWorld(io.MousePos);
                Physics::QueryFilter filter;
                filter.ignore = sim->player;
                filter.mask = ~Layer::Pickup;
                if ((to - from).length() > 0.5f) {
                    if (const auto hit = Physics::raycast(w, from, (to - from) * (GrappleRange / (to - from).length()), filter)) {
                        drawDashed(dl, view.toScreen(from), view.toScreen(hit->hit.point), withAlpha(Color::Rope, 90));
                        dl->AddCircle(view.toScreen(hit->hit.point), 5.f, withAlpha(Color::Rope, 160), 0, 1.5f);
                    }
                }
            }
        }

        // ---------------------------------------------------------------- panel

        void panel(const ImGuiIO& io) {
            ECS::Registry& w = world();
            ImGui::SetNextWindowPos({io.DisplaySize.x - PanelWidth, 0.f});
            ImGui::SetNextWindowSize({PanelWidth, io.DisplaySize.y});
            ImGui::Begin("Physics playground", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::PushTextWrapPos(0.f);
            if (ImGui::Button(paused ? "Run (P)" : "Pause (P)")) paused = !paused;
            ImGui::SameLine();
            if (ImGui::Button("Step 1/60 s")) {
                paused = true;
                sim->physics->step(w);
                afterSteps();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset (R)")) reset();
            ImGui::SliderFloat("Time scale", &timeScale, 0.05f, 2.f, "%.2f x");
            const auto& clock = w.getResource<System::PhysicsClock>();
            ImGui::Text("t = %.2f s, %llu steps; %d this frame, %.2f ms", clock.time, static_cast<unsigned long long>(clock.totalSteps), clock.stepsThisFrame, physicsMilliseconds);
            ImGui::Text("%zu contacts, %zu pairs from the broad phase", w.getResource<Resource::Contacts>().pairs.size(), w.getResource<Resource::Contacts>().candidatePairs);

            ImGui::SeparatorText("Zones (keys 1-9, 0)");
            for (int i = 0; i < 12; ++i) {
                if (i % 4 != 0) ImGui::SameLine();
                char label[32];
                std::snprintf(label, sizeof label, "%d %s", i + 1, Zones[static_cast<std::size_t>(i)].name);
                if (ImGui::Button(label, {(PanelWidth - 40.f) / 4.f, 0.f})) teleport(i);
            }

            ImGui::SeparatorText("Engine systems");
            bool changed = ImGui::Checkbox("Gravity", &enabled.gravity);
            ImGui::SameLine(150.f);
            changed |= ImGui::Checkbox("Springs", &enabled.springs);
            ImGui::SameLine(290.f);
            changed |= ImGui::Checkbox("Fields", &enabled.fields);
            changed |= ImGui::Checkbox("Drag, water", &enabled.fluids);
            ImGui::SameLine(150.f);
            changed |= ImGui::Checkbox("Friction", &enabled.friction);
            ImGui::SameLine(290.f);
            changed |= ImGui::Checkbox("Response", &enabled.response);
            changed |= ImGui::Checkbox("Continuous collision", &enabled.ccd);
            ImGui::SameLine(220.f);
            ImGui::Checkbox("Debug drawing (F1)", &debug);
            if (changed) setupPhysics();

            const CharacterController& c = controller();
            const Measures& measures = w.getResource<Measures>();
            ImGui::SeparatorText("Character (kinematic, CharacterController)");
            ImGui::Text("Velocity (%+.2f, %+.2f) m/s, facing %s", double(c.velocity.x), double(c.velocity.y), c.facing > 0 ? "right" : "left");
            if (!c.enabled) {
                ImGui::TextColored(Good, "On the rope: handed to physics (70 kg, the controller disabled)");
            } else if (c.grounded) {
                const bool onTile = c.ground == sim->level;
                ImGui::Text("Grounded on %s, normal (%.2f, %.2f)%s", onTile ? "the level" : "a body", double(c.groundNormal.x), double(c.groundNormal.y),
                            c.groundVelocity.length() > 0.f ? ", carried" : "");
            } else {
                ImGui::TextDisabled("In the air%s%s", c.wall != 0 ? (c.wall < 0 ? ", wall on the left" : ", wall on the right") : "", c.coyoteTimer > 0.f ? ", coyote time" : "");
            }
            if (c.dashing) ImGui::TextColored(Good, "Dashing");
            if (c.submerged > 0.f) {
                ImGui::TextColored(Good, "%s: %.0f %% under water (it floats in water denser than %.0f kg/m², its mass per area)",
                                   c.swimming ? "Swimming" : "Wading", double(c.submerged) * 100.0,
                                   double(c.mass / Physics::areaOf(w.getComponent<Collider>(sim->player))));
            }
            ImGui::Text("Coins: %d / 8", measures.coins);
            ImGui::Text("Jump: aimed %.2f m", double(tuning.jumpHeight));
            if (measures.lastJump >= 0.f) {
                ImGui::SameLine();
                ImGui::TextColored(Good, "last %.3f m", double(measures.lastJump));
            }
            ImGui::TextDisabled("Hold the button for a full jump; release early for a lower one. Exact under gravity: it peaks at the aimed height.");
            bool tuned = ImGui::SliderFloat("Run speed", &tuning.maxSpeed, 2.f, 14.f, "%.1f m/s");
            tuned |= ImGui::SliderFloat("Jump height", &tuning.jumpHeight, 0.5f, 6.f, "%.2f m");
            tuned |= ImGui::SliderFloat("Fall gravity", &tuning.fallGravityScale, 1.f, 3.f, "x %.2f");
            ImGui::TextDisabled("Coming down, the character's gravity is that much stronger; the world's (%.2f m/s²) is unchanged.",
                                double(-w.getResource<Resource::GravitySettings>().gravity.y));
            tuned |= ImGui::SliderFloat("Coyote time", &tuning.coyoteTime, 0.f, 0.3f, "%.2f s");
            tuned |= ImGui::SliderFloat("Dash speed", &tuning.dashSpeed, 5.f, 40.f, "%.0f m/s");
            tuned |= ImGui::Checkbox("Wall jumps", &tuning.wallJump);
            ImGui::SameLine();
            tuned |= ImGui::Checkbox("Dash", &tuning.dash);
            if (tuned) {
                CharacterController& live = controller();
                live.maxSpeed = tuning.maxSpeed;
                live.jumpHeight = tuning.jumpHeight;
                live.fallGravityScale = tuning.fallGravityScale;
                live.coyoteTime = tuning.coyoteTime;
                live.dashSpeed = tuning.dashSpeed;
                live.wallJump = tuning.wallJump;
                live.dash = tuning.dash;
            }

            ImGui::SeparatorText("Grapple");
            if (roped()) {
                const Component::Spring& s = w.getComponent<Component::Spring>(rope);
                const V end = s.other == ECS::Entity{} ? s.anchor : position(s.other);
                ImGui::Text("Rope %.2f m, %.2f m to the hook, k = %.0f N/m", double(s.restLength), double((position(sim->player) - end).length()), double(s.stiffness));
                ImGui::TextDisabled("A Spring with rope = true: it only pulls. Z or W / S reel, Q or A / D swing, Space or right click lets go.");
            } else {
                ImGui::TextDisabled("Right click within %.0f m: the first thing Physics::raycast hits holds the rope. On a body, the rope pulls it too.", double(GrappleRange));
            }

            ImGui::SeparatorText("Balls of zone 2: restitution at their last bounce");
            for (std::size_t i = 0; i < sim->balls.size(); ++i) {
                if (!alive(sim->balls[i])) continue;
                const BallProbe& probe = w.getComponent<BallProbe>(sim->balls[i]);
                const float e = w.getComponent<Collider>(sim->balls[i]).material.restitution;
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Color::Ball[i]), "e %.2f:", double(e));
                ImGui::SameLine();
                if (probe.impact >= 0.f) {
                    ImGui::Text("measured %.3f after %d bounces", double(probe.impact), probe.bounces);
                } else {
                    ImGui::TextDisabled("falling...");
                }
            }
            ImGui::TextDisabled("v out / v in: a bounce is on the speed bodies met at, not on the step's forces. Under 1 m/s it stops bouncing.");

            ImGui::SeparatorText("Explosion (E or middle click)");
            ImGui::SliderFloat("Speed at the center", &explosionSpeed, 2.f, 30.f, "%.1f m/s");
            ImGui::SliderFloat("Radius", &explosionRadius, 1.f, 10.f, "%.1f m");

            ImGui::SeparatorText("Keys");
            ImGui::TextDisabled("Q or A / D move, Z, W or Space jump, S + Space drop through, Shift dash (AZERTY or QWERTY). Right click "
                                "grapple, left drag grab, E or middle click explosion, wheel zoom. 1-9, 0 zones, F1 debug drawing, P pause, R reset.");
            ImGui::PopTextWrapPos();
            ImGui::End();
        }

        // ---------------------------------------------------------------- frame

        void frame(double realSeconds) {
            const ImGuiIO& io = ImGui::GetIO();
            sceneSize = {io.DisplaySize.x - PanelWidth, io.DisplaySize.y};
            view.middle = {sceneSize.x * 0.5f, sceneSize.y * 0.5f};
            input(io);
            updateRope();
            if (!paused) {
                const Uint64 start = SDL_GetTicksNS();
                sim->systems.update(world(), realSeconds * double(timeScale));  // the PhysicsSystem runs the steps this time holds
                physicsMilliseconds = double(SDL_GetTicksNS() - start) / 1e6;
                time += realSeconds * double(timeScale);
                afterSteps();
            }
            if (alive(sim->player)) {
                const V target = position(sim->player) + V(0.f, 1.5f);
                view.center = snapCamera ? target : view.center + (target - view.center) * 0.12f;
                snapCamera = false;
            }
            drawWorld(ImGui::GetBackgroundDrawList(), io);
            panel(io);
        }
    };

    struct Options {
        const char* screenshot = nullptr;
        int zone = 0;
        double seconds = 0.0;
        bool debug = false;
        int move = 0;
        bool jump = false;
        bool dash = false;
        std::optional<V> grapple;
        std::optional<V> explode;
    };

    Options parse(int argc, char** argv) {
        Options o;
        for (int i = 1; i < argc; ++i) {
            const auto is = [&](const char* flag) { return std::strcmp(argv[i], flag) == 0; };
            if (is("--debug")) {
                o.debug = true;
            } else if (is("--jump")) {
                o.jump = true;
            } else if (is("--dash")) {
                o.dash = true;
            } else if (i + 1 < argc && is("--screenshot")) {
                o.screenshot = argv[++i];
            } else if (i + 1 < argc && is("--at")) {
                o.zone = std::clamp(std::atoi(argv[++i]) - 1, 0, 11);
            } else if (i + 1 < argc && is("--seconds")) {
                o.seconds = std::atof(argv[++i]);
            } else if (i + 1 < argc && is("--move")) {
                o.move = std::clamp(std::atoi(argv[++i]), -1, 1);
            } else if (i + 2 < argc && (is("--grapple") || is("--explode"))) {
                const V at(static_cast<float>(std::atof(argv[i + 1])), static_cast<float>(std::atof(argv[i + 2])));
                (is("--grapple") ? o.grapple : o.explode) = at;
                i += 2;
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
    SDL_Window* window = SDL_CreateWindow("MetroV - physics playground", 1500, 880, SDL_WINDOW_RESIZABLE);
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
    demo.zone = options.zone;
    demo.debug = options.debug;
    demo.reset();
    if (options.screenshot) {  // a scripted run, step by step
        demo.paused = true;
        for (int i = 0; i < static_cast<int>(options.seconds * 60.0 + 0.5); ++i) {
            CharacterController& c = demo.controller();
            c.move = static_cast<float>(options.move);
            if (options.jump && i == 30) c.jumpPressed = true;
            c.jumpHeld = options.jump;
            if (options.dash && i == 30) c.dashPressed = true;
            if (options.grapple && i == 0) demo.grapple(*options.grapple);
            demo.updateRope();
            if (demo.roped() && !demo.controller().enabled) demo.rigid(demo.sim->player).force.x += c.move * 250.f;
            if (options.explode && i == 30) demo.explode(*options.explode);
            demo.sim->physics->step(demo.world());
            demo.time += 1.0 / 60.0;
            demo.afterSteps();
        }
        demo.snapCamera = true;
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

        SDL_SetRenderDrawColor(renderer, 24, 26, 33, 255);
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
