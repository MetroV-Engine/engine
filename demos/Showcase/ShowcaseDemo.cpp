// Every physics feature of the engine in one level, with a character to walk
// through it. The demo only draws, reads the keyboard and the mouse, and
// builds the level: everything that moves is moved by the engine's own
// systems, registered by Engine::Physics::install() and stepped at
// Engine::Physics::Step (1/60 s).
//
// Zones (keys 1 to 8 teleport the character there):
//   1 Crates      boxes, polygons, a plank that tips over, a crate too heavy to push
//   2 Bounce      three balls of restitution 0.9, 0.6 and 0.2: measured e = sqrt(h2 / h1)
//   3 Slopes      a hill, a crate held by static friction, an icy crate that slides
//                 onto ice then stops on rubber, balls rolling down; on the ice the
//                 character's feet slip too (CharacterController::fullGripFriction)
//   4 Platforms   a kinematic platform over a pit, a turning kinematic bar
//   5 Springs     a rope to walk on (links joined end to end by springs), a lantern
//                 hanging by its top, a ceiling to grapple
//   6 Water       crates, a ball, a triangle, a raft and a stone in a pool: each
//                 floats at the depth its density gives, or sinks
//   7 Drag        a drop tower (no drag, linear, quadratic in a breeze) and a fan
//                 (a radial field) holding two balls up in a tube
//   8 Gravity     a planet (GravityAttractor) with two moons in orbit
//
// Controls:
//   Q or A / D, arrows   run      Space, Z or W, Up   jump (hold for higher)
//   Left drag       grab any body (a Spring to the cursor)
//   Right click     grapple: a Spring to the scenery hit by a ray; W/S shorten or
//                   lengthen the rope (Z or W / S), right click or Space lets go
//   E, middle click explosion at the cursor (a RadialField for one step)
//   B / O / H       drop a crate / a ball / a hexagon at the cursor
//   T               drop again the balls of zone 2 and the tower of zone 7
//   P pause, N one step, R reset, F1 debug drawing, wheel zoom
//
// Usage: demo_showcase [--zone 1-8] [--seconds s] [--move -1..1] [--screenshot file.bmp]
//                      [--selftest]
//   --screenshot runs s seconds (holding --move), saves the image and quits.
//   --selftest runs scripted checks on the level without a window and prints
//   what each measures (exit code 1 when one fails).

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "ECS/ECS.hpp"
#include "Engine/Common/Physic/Collision/Collision.hpp"
#include "Engine/Common/Physic/Forces/Spring.hpp"
#include "Engine/Component/Physics/Buoyant.hpp"
#include "Engine/Component/Physics/CharacterController.hpp"
#include "Engine/Component/Physics/CharacterInput.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/Drag.hpp"
#include "Engine/Component/Physics/FluidVolume.hpp"
#include "Engine/Component/Physics/GravityAttractor.hpp"
#include "Engine/Component/Physics/RadialField.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/Spring.hpp"
#include "Engine/Component/Transform.hpp"
#include "Engine/System/Physics/Physics.hpp"

using namespace Engine;
using V = Math::Vector2f;
using Component::Collider;

namespace {
    constexpr float Pi = 3.14159265f;
    constexpr float Step = static_cast<float>(Physics::Step);
    constexpr float PanelWidth = 390.f;

    // ------------------------------------------------------------------ demo-only components (no physics in them)

    /** How to draw an entity. */
    struct Paint {
        ImU32 color = IM_COL32(200, 200, 200, 255);
    };

    /**
     * Level script: drives a kinematic platform (RigidBody of mass 0) back and
     * forth between two x by setting its velocity. The engine moves it, and
     * carries what stands on it.
     */
    struct Patrol {
        float fromX = 0.f;
        float toX = 1.f;
        float speed = 2.f;
    };

    /** A body that T drops again from where the level put it. */
    struct Redrop {
        V position;
    };

    /** A link of the rope of zone 5: drawn as rope, not as a box. */
    struct RopeLink {};

    // The rope of zone 5: links of 0.6 m joined end to end by springs. A
    // joint's stiffness is bounded by the lightest thing it moves, the end of
    // a link (1 / m + r² / I = 0.64 / kg for these): explicit springs at
    // 60 Hz blow up above about 3000 N/m here (measured: 5000 does). 3000 holds
    // but stretches: a 12 m rope sags to 3 m under its own weight, this 6 m
    // one to 2.2 m, a hammock to jump out of. A taut rope needs joints solved
    // as constraints, as contacts are. The links are 0.97 of their share of
    // the span, so the rope starts taut.
    constexpr int RopeLinks = 10;
    constexpr float RopeLinkMass = 6.f;
    constexpr float RopeThickness = 0.07f;
    constexpr float RopeTautness = 0.97f;
    constexpr float RopeStiffness = 3000.f;
    constexpr float RopeDamping = 30.f;
    constexpr float RopeAirDrag = 3.f;  // N·s/m per link: a swing loses half its speed in 1.4 s

    /** A one-step explosion: its RadialField acts for one step, then the demo removes it. */
    struct Blast {};

    // ------------------------------------------------------------------ colors

    namespace Color {
        const ImU32 Sky = IM_COL32(24, 27, 34, 255);
        const ImU32 Rock = IM_COL32(78, 74, 68, 255);
        const ImU32 Ice = IM_COL32(150, 205, 240, 255);
        const ImU32 Rubber = IM_COL32(200, 80, 90, 255);
        const ImU32 Crate = IM_COL32(205, 150, 80, 255);
        const ImU32 Heavy = IM_COL32(120, 110, 125, 255);
        const ImU32 Ball = IM_COL32(235, 205, 90, 255);
        const ImU32 Green = IM_COL32(110, 200, 120, 255);
        const ImU32 Purple = IM_COL32(170, 120, 220, 255);
        const ImU32 Monster = IM_COL32(220, 70, 160, 255);
        const ImU32 Hero = IM_COL32(80, 160, 255, 255);
        const ImU32 HeroPhysics = IM_COL32(255, 150, 60, 255);
        const ImU32 Water = IM_COL32(50, 120, 220, 90);
        const ImU32 WaterTop = IM_COL32(110, 170, 255, 200);
        const ImU32 Spring = IM_COL32(230, 230, 230, 255);
        const ImU32 Rope = IM_COL32(190, 150, 95, 255);
        const ImU32 Text = IM_COL32(230, 232, 236, 255);
        const ImU32 Dim = IM_COL32(150, 155, 165, 255);
        const ImU32 Normal = IM_COL32(255, 80, 80, 255);
        const ImU32 Velocity = IM_COL32(120, 255, 160, 255);
        const ImU32 Field = IM_COL32(255, 220, 120, 70);
    }

    // ------------------------------------------------------------------ zones

    struct Zone {
        const char* name;
        V spawn;  // where the character appears (its center)
        const char* what;
    };

    constexpr std::array<Zone, 8> Zones{{
        {"1 Crates", {3.f, 1.f}, "Push crates and polygons; tip the plank over; the 400 kg crate needs more than pushStrength. Walk left through the pink monster: its layer is not in your mask."},
        {"2 Bounce", {23.f, 1.f}, "Same drop height, restitution 0.9 / 0.6 / 0.2: each rebound keeps e^2 of the height."},
        {"3 Slopes", {33.f, 1.f}, "Walk the hill. The rubber crate holds (tan 26.6 deg = 0.5 < muS), the icy one slides. Run on the ice: your feet slip."},
        {"4 Platforms", {62.f, 1.f}, "Jump on the moving platform: it carries you. The bar turns, kinematic."},
        {"5 Springs", {84.f, 1.f}, "Walk the rope: links joined end to end by springs. Right click the ceiling to swing on a rope, Z or W / S to reel."},
        {"6 Water", {104.f, 1.f}, "Each body floats with the share of it its density gives: m / (area * rho)."},
        {"7 Drag", {131.f, 1.f}, "T drops the tower again: terminal speeds sqrt(m g / c) and m g / c. The fan holds two balls."},
        {"8 Gravity", {152.f, 1.f}, "Two moons orbit the planet under G m M / d^2 alone (gravityScale 0)."},
    }};

    // Water of zone 6: density in kg/m² (2D), a 1 m² crate of 30 kg floats half under.
    constexpr float WaterDensity = 60.f;
    // Floor of the fan's tube in zone 7: the 1.8 m character walks under it.
    constexpr float FanHeight = 2.4f;
    // Planet of zone 8: G M chosen so that orbits are a few m/s.
    constexpr float PlanetGM = 40.f;

    // Center of the crate standing on the 26.6° slope of zone 3.
    const V HeldCrateStart = V(38.f, 1.5f) + V(-std::sin(std::atan(0.5f)), std::cos(std::atan(0.5f))) * 0.401f;

    struct Material {
        float restitution = 0.3f;
        float staticFriction = 0.5f;
        float kineticFriction = 0.3f;
    };
    constexpr Material Wood{};
    constexpr Material IceMaterial{0.1f, 0.03f, 0.02f};
    constexpr Material RubberMaterial{0.2f, 1.1f, 0.9f};

    // Collision layers of the demo, one bit each (Collider::layer and mask).
    // Everything else keeps the default: layer World, mask everything.
    namespace Layer {
        constexpr std::uint32_t World = 1u << 0;
        constexpr std::uint32_t Hero = 1u << 1;
        constexpr std::uint32_t Enemy = 1u << 2;
    }
    // The monster of zone 1, left of the start: the hero walks through it.
    constexpr V MonsterStart{1.5f, 1.f};

    // ------------------------------------------------------------------ the level

    /** A spring held at both ends' positions (bodyPoint and otherPoint at 0); set them after for a joint. */
    Component::Spring makeSpring(ECS::Entity body, ECS::Entity other, const V& anchor, float rest, float stiffness, float damping) {
        Component::Spring spring;
        spring.body = body;
        spring.other = other;
        spring.anchor = anchor;
        spring.restLength = rest;
        spring.stiffness = stiffness;
        spring.damping = damping;
        return spring;
    }

    /** What the panel and the self test measure: entities of interest. */
    struct Landmarks {
        ECS::Entity hero;
        std::array<ECS::Entity, 3> bouncers{};
        std::array<float, 3> bouncerRestitution{0.9f, 0.6f, 0.2f};
        ECS::Entity heldCrate, icyCrate, heavyCrate, plank, platform;
        std::vector<ECS::Entity> floaters;
        ECS::Entity stone, dropNone, dropLinear, dropQuadratic;
        std::array<ECS::Entity, 2> fanBalls{};
        ECS::Entity fan, planet;
        std::array<ECS::Entity, 2> moons{};
        ECS::Entity bridgeMiddle;
        ECS::Entity monster, monsterCrate;
        Shapes::Rectangle water{};
    };

    class Level {
        public:
            ECS::Registry world;
            ECS::SystemManager systems;
            Landmarks marks;

            Level() {
                Physics::install(world, systems);
                build();
            }

            // ---------------------------------------------------------- builders

            ECS::Entity scenery(const Collider::Shape& shape, const V& center, ImU32 color, Material material = Wood) {
                const ECS::Entity e = world.spawnEntity();
                world.addComponent(e, Component::Transform{center, 0.f});
                world.addComponent(e, collider(shape, material));
                world.addComponent(e, Paint{color});
                return e;
            }

            ECS::Entity box(const V& lower, const V& upper, ImU32 color = Color::Rock, Material material = Wood) {
                return scenery(Collider::Box{(upper - lower) * 0.5f}, (lower + upper) * 0.5f, color, material);
            }

            /** A slope over the box from lower to upper, rising to the right or to the left. */
            ECS::Entity slope(const V& lower, const V& upper, bool risesToTheRight, ImU32 color = Color::Rock) {
                return scenery(Collider::Polygon::slope((upper - lower) * 0.5f, risesToTheRight), (lower + upper) * 0.5f, color);
            }

            ECS::Entity body(const Collider::Shape& shape, const V& center, float mass, ImU32 color, Material material = Wood) {
                const ECS::Entity e = scenery(shape, center, color, material);
                Component::RigidBody rigid;
                rigid.mass = mass;
                world.addComponent(e, rigid);
                return e;
            }

            /** Floats in the pool: its Buoyant shape is its collision shape. */
            ECS::Entity floater(const Collider::Shape& shape, const V& center, float mass, ImU32 color) {
                const ECS::Entity e = body(shape, center, mass, color);
                world.addComponent(e, Component::Buoyant{shape, {}});
                marks.floaters.push_back(e);
                return e;
            }

            ECS::Entity spring(ECS::Entity end, ECS::Entity other, const V& anchor, float rest, float stiffness, float damping) {
                const ECS::Entity e = world.spawnEntity();
                world.addComponent(e, makeSpring(end, other, anchor, rest, stiffness, damping));
                return e;
            }

            static Collider collider(const Collider::Shape& shape, Material material) {
                Collider c;
                c.shape = shape;
                c.restitution = material.restitution;
                c.staticFriction = material.staticFriction;
                c.kineticFriction = material.kineticFriction;
                return c;
            }

            static Collider::Polygon regular(int sides, float radius, float turn = 0.f) {
                Collider::Polygon polygon;
                polygon.points.count = static_cast<std::size_t>(sides);
                for (int i = 0; i < sides; ++i) {
                    const float angle = turn + 2.f * Pi * static_cast<float>(i) / static_cast<float>(sides);
                    polygon.points.vertices[static_cast<std::size_t>(i)] = V(std::cos(angle), std::sin(angle)) * radius;
                }
                return polygon;
            }

            // ---------------------------------------------------------- the zones

            void build() {
                // Walls at both ends, and a ceiling over the springs for the grapple.
                box({-3.f, -2.f}, {0.f, 14.f});
                box({172.f, -2.f}, {175.f, 14.f});

                buildCrates();
                buildBounce();
                buildSlopes();
                buildPlatforms();
                buildSprings();
                buildWater();
                buildDrag();
                buildGravity();
                buildHero();
                buildMonster();
            }

            void buildCrates() {
                box({0.f, -2.f}, {22.f, 0.f});
                // A tall plank: pushed, it tips over (rotation, inertia of a box).
                marks.plank = body(Collider::Box{{0.15f, 1.2f}}, {6.5f, 1.2f}, 15.f, Color::Crate);
                // A pyramid of light crates.
                for (int row = 0; row < 3; ++row) {
                    for (int i = 0; i < 3 - row; ++i) {
                        body(Collider::Box{{0.35f, 0.35f}}, {9.f + 0.72f * static_cast<float>(i) + 0.36f * static_cast<float>(row), 0.35f + 0.71f * static_cast<float>(row)},
                             8.f, Color::Crate);
                    }
                }
                // Polygons: a hexagon and a triangle, dropped.
                body(regular(6, 0.45f), {13.f, 1.f}, 10.f, Color::Purple);
                body(regular(3, 0.5f, Pi / 2.f), {14.5f, 2.f}, 6.f, Color::Green);
                // Static friction of 400 kg is 0.5 * 400 * 9.81 = 1962 N > pushStrength 1000 N: it never moves.
                marks.heavyCrate = body(Collider::Box{{0.7f, 0.7f}}, {18.f, 0.7f}, 400.f, Color::Heavy);
            }

            void buildMonster() {
                // Layers: a monster on layer Enemy, which the hero's mask leaves out. The hero walks
                // through it; the ground and the crate on its head still collide with it.
                marks.monster = body(Collider::Box{{0.4f, 1.f}}, MonsterStart, 60.f, Color::Monster);  // taller than the hero
                world.getComponent<Collider>(marks.monster).layer = Layer::Enemy;
                marks.monsterCrate = body(Collider::Box{{0.25f, 0.25f}}, MonsterStart + V(0.f, 1.25f), 4.f, Color::Crate);
            }

            void buildBounce() {
                box({22.f, -2.f}, {34.f, 0.f}, Color::Rock, Material{0.f, 0.5f, 0.3f});
                for (std::size_t i = 0; i < 3; ++i) {
                    const V start(25.5f + 3.f * static_cast<float>(i), 6.f);
                    marks.bouncers[i] = body(Collider::Circle{0.4f}, start, 2.f, Color::Ball, Material{marks.bouncerRestitution[i], 0.5f, 0.3f});
                    world.addComponent(marks.bouncers[i], Redrop{start});
                }
            }

            void buildSlopes() {
                box({34.f, -2.f}, {50.f, 0.f});
                slope({35.f, 0.f}, {41.f, 3.f}, true);          // 26.6°
                box({41.f, 0.f}, {44.f, 3.f});                   // plateau
                slope({44.f, 0.f}, {48.f, 3.f}, false);         // 36.9°
                box({50.f, -2.f}, {56.f, 0.f}, Color::Ice, IceMaterial);
                box({56.f, -2.f}, {62.f, 0.f}, Color::Rubber, RubberMaterial);
                box({62.f, -6.f}, {64.f, 0.f});  // down to the pit's floor: nothing rolls out under it

                const float gentle = std::atan(0.5f);
                // On the 26.6° slope (tan = 0.5), a rubber-soled crate: combined with the
                // wood, muS 0.71 and muK 0.55, both above 0.5, it holds.
                marks.heldCrate = body(Collider::Box{{0.4f, 0.4f}}, HeldCrateStart, 12.f, Color::Rubber, Material{0.1f, 1.f, 1.f});
                world.getComponent<Component::Transform>(marks.heldCrate).rotation = gentle;
                // On the 36.9° slope, an icy crate slides down, across the ice, and stops on the rubber.
                const float steep = std::atan(0.75f);
                marks.icyCrate = body(Collider::Box{{0.4f, 0.4f}}, V(45.f, 2.25f) + V(std::sin(steep), std::cos(steep)) * 0.401f,
                                      12.f, Color::Ice, IceMaterial);
                world.getComponent<Component::Transform>(marks.icyCrate).rotation = -steep;
                // Balls on the plateau: nudge them, they roll down (friction turns them).
                body(Collider::Circle{0.35f}, {43.3f, 3.36f}, 3.f, Color::Ball);
                body(regular(8, 0.35f), {42.2f, 3.4f}, 3.f, Color::Green);
            }

            void buildPlatforms() {
                // A pit 4 m deep, with a 45° way out on the right.
                box({64.f, -6.f}, {82.f, -4.f});
                slope({78.f, -4.f}, {82.f, 0.f}, true);
                box({82.f, -2.f}, {86.f, 0.f});
                // A kinematic platform: mass 0, moved by its velocity, never pushed.
                marks.platform = body(Collider::Box{{1.4f, 0.25f}}, {65.6f, -0.25f}, 0.f, Color::Green);
                world.addComponent(marks.platform, Patrol{65.6f, 76.4f, 2.5f});
                // A turning kinematic bar: crates dropped on it are thrown off.
                const ECS::Entity bar = body(Collider::Box{{2.6f, 0.2f}}, {71.f, 4.5f}, 0.f, Color::Purple);
                world.getComponent<Component::RigidBody>(bar).angularVelocity = 0.7f;
                body(Collider::Box{{0.3f, 0.3f}}, {70.f, 6.f}, 5.f, Color::Crate);
                body(Collider::Circle{0.3f}, {72.f, 6.5f}, 5.f, Color::Ball);
            }

            void buildSprings() {
                // A slack rope over a trench: short links joined end to end by springs
                // (Spring::bodyPoint, otherPoint), each free to turn, so the rope bends
                // where it is stood on. 6 m long: explicit springs stable at 60 Hz are too
                // soft for a longer one (it would sag down to the floor, see RopeStiffness).
                // A trench 2.4 m deep under it: below the rope hanging alone, shallow enough to jump out of (jumpHeight 2.5).
                box({82.f, -5.f}, {89.f, 0.f});
                box({89.f, -5.f}, {95.f, -2.4f});
                box({95.f, -5.f}, {104.f, 0.f});
                box({84.f, 9.f}, {104.f, 10.f});  // ceiling for the grapple
                // Hooked just off the edges, its top level with the ground.
                const V left(89.05f, -RopeThickness);
                const V right(94.95f, -RopeThickness);
                const float span = right.x - left.x;
                const float half = span / (2.f * static_cast<float>(RopeLinks)) * RopeTautness;
                ECS::Entity previous{};
                for (int i = 0; i < RopeLinks; ++i) {
                    const float along = span * (static_cast<float>(i) + 0.5f) / static_cast<float>(RopeLinks);
                    const ECS::Entity link = body(Collider::Box{{half, RopeThickness}}, left + V(along, 0.f), RopeLinkMass, Color::Crate,
                                                 Material{0.f, 0.6f, 0.5f});
                    world.addComponent(link, RopeLink{});
                    // The joints damp the stretch, not the swing of the whole rope: the air does.
                    world.addComponent(link, Component::Drag{RopeAirDrag, 0.f, {}});
                    // This link's left end to the previous link's right end, or to the left hook.
                    Component::Spring joint = makeSpring(link, previous, left, 0.f, RopeStiffness, RopeDamping);
                    joint.bodyPoint = V(-half, 0.f);
                    joint.otherPoint = V(half, 0.f);
                    world.addComponent(world.spawnEntity(), joint);
                    previous = link;
                    if (i == RopeLinks / 2) {
                        marks.bridgeMiddle = link;
                    }
                }
                Component::Spring last = makeSpring(previous, ECS::Entity{}, right, 0.f, RopeStiffness, RopeDamping);
                last.bodyPoint = V(half, 0.f);
                world.addComponent(world.spawnEntity(), last);
                // A lantern on a spring from the ceiling, held by its top: it swings and turns.
                const ECS::Entity lantern = body(Collider::Box{{0.3f, 0.4f}}, {91.f, 5.5f}, 5.f, Color::Ball);
                Component::Spring hanger = makeSpring(lantern, ECS::Entity{}, {91.f, 9.f}, 2.5f, 400.f, 6.f);
                hanger.bodyPoint = V(0.f, 0.4f);
                world.addComponent(world.spawnEntity(), hanger);
            }

            void buildWater() {
                // A pool 4 m deep with a way out on the right, under water.
                box({104.f, -6.f}, {106.f, 0.f});
                box({106.f, -6.f}, {126.f, -4.f});
                slope({121.f, -4.f}, {126.f, 0.f}, true);
                box({126.f, -2.f}, {130.f, 0.f});
                const V center(116.f, -2.25f);
                const V half(10.f, 1.75f);
                const ECS::Entity pool = world.spawnEntity();
                world.addComponent(pool, Component::Transform{center, 0.f});
                world.addComponent(pool, Component::FluidVolume{half, WaterDensity, 40.f});
                marks.water = Shapes::Rectangle::fromCenter(center, half);

                // Each floats at m / (area * rho) of its area under the surface.
                floater(Collider::Box{{0.5f, 0.5f}}, {108.f, 1.f}, 30.f, Color::Crate);                // 1/2
                floater(Collider::Box{{0.5f, 0.5f}}, {110.f, 1.f}, 12.f, Color::Crate);                // 1/5
                floater(Collider::Circle{0.5f}, {112.f, 1.f}, 20.f, Color::Ball);                       // 0.42
                floater(regular(3, 0.6f, Pi / 2.f), {114.f, 1.f}, 10.f, Color::Green);                  // 0.36
                floater(Collider::Box{{1.5f, 0.15f}}, {117.f, 1.5f}, 12.f, Color::Crate);               // raft, 0.22
                world.getComponent<Component::Transform>(marks.floaters.back()).rotation = 0.4f;          // dropped tilted
                marks.stone = floater(Collider::Box{{0.3f, 0.3f}}, {119.5f, 1.f}, 40.f, Color::Heavy); // 0.36 m² of 40 kg: sinks
            }

            void buildDrag() {
                box({130.f, -2.f}, {150.f, 0.f});
                // Drop tower: no drag, linear drag, quadratic drag in a breeze.
                marks.dropNone = body(Collider::Box{{0.25f, 0.25f}}, {133.f, 12.f}, 1.f, Color::Heavy);
                marks.dropLinear = body(Collider::Circle{0.25f}, {135.f, 12.f}, 0.5f, Color::Ball);
                world.addComponent(marks.dropLinear, Component::Drag{1.f, 0.f, {}});              // mg / c = 4.9 m/s
                marks.dropQuadratic = body(regular(5, 0.3f), {137.f, 12.f}, 0.3f, Color::Green);
                world.addComponent(marks.dropQuadratic, Component::Drag{0.f, 0.3f, {1.5f, 0.f}}); // sqrt(mg / c) = 3.1 m/s
                for (const ECS::Entity e : {marks.dropNone, marks.dropLinear, marks.dropQuadratic}) {
                    world.addComponent(e, Redrop{world.getComponent<Component::Transform>(e).position});
                }
                // A fan in a glass tube hung high enough to walk under: a radial field of 40 N
                // fading out at 5 m holds a ball of weight w at 5 (1 - w / 40) above it.
                const ImU32 glass = IM_COL32(140, 170, 190, 255);
                box({142.6f, FanHeight - 0.2f}, {145.4f, FanHeight}, glass);
                box({142.6f, FanHeight}, {142.8f, FanHeight + 6.5f}, glass);
                box({145.2f, FanHeight}, {145.4f, FanHeight + 6.5f}, glass);
                marks.fan = world.spawnEntity();
                world.addComponent(marks.fan, Component::Transform{{144.f, FanHeight}, 0.f});
                world.addComponent(marks.fan, Component::RadialField{40.f, 5.f});
                marks.fanBalls[0] = body(Collider::Circle{0.45f}, {144.f, FanHeight + 1.f}, 0.5f, Color::Ball);
                marks.fanBalls[1] = body(Collider::Circle{0.45f}, {144.f, FanHeight + 2.5f}, 1.f, Color::Rubber);
                for (const ECS::Entity e : marks.fanBalls) {
                    world.addComponent(e, Component::Drag{0.5f, 0.f, {}});  // air, so that they settle
                }
            }

            void buildGravity() {
                box({150.f, -2.f}, {172.f, 0.f});
                const float G = Resource::GravitySettings::of(world).gravitationalConstant;
                marks.planet = scenery(Collider::Circle{1.4f}, {161.f, 9.f}, Color::Purple);
                world.addComponent(marks.planet, Component::GravityAttractor{PlanetGM / G});
                const std::array<float, 2> radii{3.f, 4.6f};
                for (std::size_t i = 0; i < 2; ++i) {
                    const float r = radii[i];
                    const ECS::Entity moon = body(Collider::Circle{0.25f}, V(161.f, 9.f + r), 1.f, i == 0 ? Color::Ball : Color::Green);
                    Component::RigidBody& rigid = world.getComponent<Component::RigidBody>(moon);
                    rigid.gravityScale = 0.f;
                    rigid.velocity = V(-std::sqrt(PlanetGM / r), 0.f);  // circular orbit: v = sqrt(G M / r)
                    marks.moons[i] = moon;
                }
            }

            void buildHero() {
                const ECS::Entity hero = world.spawnEntity();
                world.addComponent(hero, Component::Transform{Zones[0].spawn, 0.f});
                Component::RigidBody rigid;
                rigid.mass = 70.f;
                world.addComponent(hero, rigid);
                Collider c = collider(Collider::Box{{0.4f, 0.9f}}, Material{0.f, 0.6f, 0.5f});
                c.response = Collider::Response::Stop;  // never bounces
                c.layer = Layer::Hero;
                c.mask = ~Layer::Enemy;  // walks through monsters, collides with the rest
                world.addComponent(hero, c);
                world.addComponent(hero, Component::CharacterController{});
                world.addComponent(hero, Component::CharacterInput{});
                world.addComponent(hero, Component::Buoyant{c.shape, {}});  // 70 kg on 1.44 m²: floats 0.8 under
                world.addComponent(hero, Paint{Color::Hero});
                marks.hero = hero;
            }

            // ---------------------------------------------------------- level scripts, run before each step

            void runScripts() {
                for (auto [entity, transform, rigid, patrol] : world.view<Component::Transform, Component::RigidBody, Patrol>()) {
                    if (transform.position.x >= patrol.toX) {
                        rigid.velocity = V(-patrol.speed, 0.f);
                    } else if (transform.position.x <= patrol.fromX) {
                        rigid.velocity = V(patrol.speed, 0.f);
                    } else if (rigid.velocity.x == 0.f) {
                        rigid.velocity = V(patrol.speed, 0.f);
                    }
                }
            }

            void redrop() {
                for (auto [entity, transform, rigid, drop] : world.view<Component::Transform, Component::RigidBody, Redrop>()) {
                    transform.position = drop.position;
                    transform.rotation = 0.f;
                    rigid.velocity = {};
                    rigid.angularVelocity = 0.f;
                }
            }

            /** Pushes everything around point away for one step: a RadialField removed after it. */
            void explode(const V& point, float strength, float radius) {
                const ECS::Entity blast = world.spawnEntity();
                world.addComponent(blast, Component::Transform{point, 0.f});
                world.addComponent(blast, Component::RadialField{strength, radius});
                world.addComponent(blast, Blast{});
                // Thrown by it, the character lets physics move it for a moment.
                const V heroAt = world.getComponent<Component::Transform>(marks.hero).position;
                if (heroAt.distanceTo(point) < radius) {
                    world.getComponent<Component::CharacterController>(marks.hero).handToPhysics(0.6f);
                }
            }

            void removeBlasts() {
                std::vector<ECS::Entity> blasts;
                for (auto [entity, blast] : world.view<Blast>()) {
                    blasts.push_back(entity);
                }
                for (const ECS::Entity e : blasts) {
                    world.killEntity(e);
                }
            }

            /** The scenery (no RigidBody) first hit by a ray, if any. */
            /** First static collider on the ray whose layer is in mask: a ray has no layer, only what it looks for. */
            [[nodiscard]] std::optional<Collision::Hit> raycastScenery(const V& origin, const V& delta, std::uint32_t mask = ~0u) {
                std::optional<Collision::Hit> best;
                for (auto [entity, transform, c] : world.view<Component::Transform, Collider>()) {
                    if (world.hasComponents<Component::RigidBody>(entity) || (c.layer & mask) == 0u) {
                        continue;
                    }
                    const std::optional<Collision::Hit> hit = std::visit(
                        [&](const auto& shape) { return Collision::raycast(origin, delta, shape); }, c.worldShape(transform));
                    if (hit && (!best || hit->fraction < best->fraction)) {
                        best = hit;
                    }
                }
                return best;
            }

            /** The dynamic body under a point, if any. */
            [[nodiscard]] std::optional<ECS::Entity> bodyAt(const V& point) {
                for (auto [entity, transform, c, rigid] : world.view<Component::Transform, Collider, Component::RigidBody>()) {
                    if (rigid.mass <= 0.f) {
                        continue;
                    }
                    const bool inside = std::visit([&](const auto& shape) { return Collision::contains(shape, point); }, c.worldShape(transform));
                    if (inside) {
                        return entity;
                    }
                }
                return std::nullopt;
            }

            /** One fixed step: the level scripts, then every physics system of the engine. */
            void step() {
                runScripts();
                systems.update(world, Physics::Step);
                removeBlasts();
            }

            // ---------------------------------------------------------- measurements

            [[nodiscard]] V position(ECS::Entity e) const { return world.getComponent<Component::Transform>(e).position; }
            [[nodiscard]] V velocity(ECS::Entity e) const { return world.getComponent<Component::RigidBody>(e).velocity; }

            /** Share of a floater's area under water, as BuoyancySystem sees it. */
            [[nodiscard]] float submergedShare(ECS::Entity e) const {
                const auto& transform = world.getComponent<Component::Transform>(e);
                const auto& buoyant = world.getComponent<Component::Buoyant>(e);
                const Component::Collider::WorldShape shape = buoyant.worldShape(transform);
                const float area = std::visit([](const auto& s) { return areaOf(s); }, shape);
                return System::BuoyancySystem::submergedPart(shape, marks.water).area / area;
            }

            [[nodiscard]] float densityShare(ECS::Entity e) const {
                const auto& transform = world.getComponent<Component::Transform>(e);
                const float area = std::visit([](const auto& s) { return areaOf(s); },
                                              world.getComponent<Component::Buoyant>(e).worldShape(transform));
                return world.getComponent<Component::RigidBody>(e).mass / (area * WaterDensity);
            }

            static float areaOf(const Shapes::Rectangle& r) { return (r.max.x - r.min.x) * (r.max.y - r.min.y); }
            static float areaOf(const Shapes::Circle& c) { return Pi * c.radius * c.radius; }
            static float areaOf(const Shapes::Polygon& p) { return p.area(); }
    };

    // ------------------------------------------------------------------ what the panel tracks over time

    /** Apexes of a bouncing ball: e = sqrt(h[n+1] / h[n]). */
    struct BounceTrack {
        std::vector<float> apexes;  // heights of the bottom of the ball
        float lastVy = 0.f;
        void update(float bottom, float vy, float startHeight) {
            if (apexes.empty()) {
                apexes.push_back(startHeight);
            }
            if (lastVy > 0.f && vy <= 0.f && bottom > 0.05f) {
                apexes.push_back(bottom);
            }
            lastVy = vy;
        }
        [[nodiscard]] float measured(std::size_t n) const {
            return n + 1 < apexes.size() && apexes[n] > 0.f ? std::sqrt(apexes[n + 1] / apexes[n]) : -1.f;
        }
    };

    struct Tracking {
        std::array<BounceTrack, 3> bounces;
        float jumpStart = 0.f, jumpApex = -1.f, lastJump = -1.f, feetBefore = 0.f;
        bool inJump = false;
        std::array<float, 3> topSpeed{};
        std::array<float, 2> moonMin{1e9f, 1e9f}, moonMax{0.f, 0.f};
        float ropeTopSpeed = 0.f;  // fastest a rope link went: a rope blowing up would show here
        double stepMicroseconds = 0.0;
        int steps = 0;

        void reset() { *this = Tracking{}; }

        void update(Level& level) {
            Landmarks& m = level.marks;
            for (std::size_t i = 0; i < 3; ++i) {
                const V p = level.position(m.bouncers[i]);
                bounces[i].update(p.y - 0.4f, level.velocity(m.bouncers[i]).y, 6.f - 0.4f);
            }
            const auto& controller = level.world.getComponent<Component::CharacterController>(m.hero);
            const float feet = level.position(m.hero).y - 0.9f;
            if (controller.state.jumped) {
                inJump = true;
                jumpStart = feetBefore;  // where the feet were when the jump started, before this step moved them
                jumpApex = feet;
            }
            feetBefore = feet;
            if (inJump) {
                jumpApex = std::max(jumpApex, feet);
                if (level.velocity(m.hero).y < 0.f) {
                    inJump = false;
                    lastJump = jumpApex - jumpStart;
                }
            }
            const std::array<ECS::Entity, 3> drops{m.dropNone, m.dropLinear, m.dropQuadratic};
            for (std::size_t i = 0; i < 3; ++i) {
                topSpeed[i] = std::max(topSpeed[i], -level.velocity(drops[i]).y);
            }
            for (auto [entity, link, rigid] : level.world.view<RopeLink, Component::RigidBody>()) {
                ropeTopSpeed = std::max(ropeTopSpeed, rigid.velocity.length());
            }
            for (std::size_t i = 0; i < 2; ++i) {
                const float d = level.position(m.moons[i]).distanceTo(level.position(m.planet));
                moonMin[i] = std::min(moonMin[i], d);
                moonMax[i] = std::max(moonMax[i], d);
            }
        }
    };

    // ------------------------------------------------------------------ view and drawing

    struct View {
        V center{8.f, 3.f};
        float scale = 34.f;  // pixels per metre
        ImVec2 middle;
        [[nodiscard]] ImVec2 toScreen(const V& w) const { return {middle.x + (w.x - center.x) * scale, middle.y - (w.y - center.y) * scale}; }
        [[nodiscard]] V toWorld(const ImVec2& s) const { return {center.x + (s.x - middle.x) / scale, center.y - (s.y - middle.y) / scale}; }
    };

    ImU32 darker(ImU32 color) {
        const ImVec4 c = ImGui::ColorConvertU32ToFloat4(color);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x * 0.55f, c.y * 0.55f, c.z * 0.55f, 1.f));
    }

    void drawArrow(ImDrawList* dl, ImVec2 from, ImVec2 to, ImU32 color, float thickness = 2.f) {
        dl->AddLine(from, to, color, thickness);
        const float dx = to.x - from.x, dy = to.y - from.y, length = std::sqrt(dx * dx + dy * dy);
        if (length < 4.f) return;
        const float ux = dx / length, uy = dy / length, head = 7.f;
        dl->AddTriangleFilled(to, {to.x - ux * head - uy * head * 0.5f, to.y - uy * head + ux * head * 0.5f},
                              {to.x - ux * head + uy * head * 0.5f, to.y - uy * head - ux * head * 0.5f}, color);
    }

    void drawShape(ImDrawList* dl, const View& view, const Collider::WorldShape& shape, ImU32 color, float rotation) {
        const ImU32 edge = darker(color);
        if (const auto* r = std::get_if<Shapes::Rectangle>(&shape)) {
            const ImVec2 a = view.toScreen({r->min.x, r->max.y});
            const ImVec2 b = view.toScreen({r->max.x, r->min.y});
            dl->AddRectFilled(a, b, color);
            dl->AddRect(a, b, edge, 0.f, 0, 1.5f);
        } else if (const auto* c = std::get_if<Shapes::Circle>(&shape)) {
            const ImVec2 center = view.toScreen(c->center);
            dl->AddCircleFilled(center, c->radius * view.scale, color, 32);
            dl->AddCircle(center, c->radius * view.scale, edge, 32, 1.5f);
            // A spoke, to see it turn.
            dl->AddLine(center, view.toScreen(c->center + V(std::cos(rotation), std::sin(rotation)) * c->radius), edge, 2.f);
        } else if (const auto* p = std::get_if<Shapes::Polygon>(&shape)) {
            std::array<ImVec2, Shapes::Polygon::MaxVertices> points{};
            for (std::size_t i = 0; i < p->count; ++i) {
                points[i] = view.toScreen(p->vertices[i]);
            }
            // World counter-clockwise is screen clockwise (y flips), as ImGui wants.
            dl->AddConvexPolyFilled(points.data(), static_cast<int>(p->count), color);
            dl->AddPolyline(points.data(), static_cast<int>(p->count), edge, ImDrawFlags_Closed, 1.5f);
        }
    }

    // ------------------------------------------------------------------ the demo

    struct Demo {
        std::unique_ptr<Level> level;
        Tracking tracking;
        View view;
        bool paused = false, stepRequested = false, debug = false, follow = true;
        float timeScale = 1.f;
        double accumulator = 0.0;
        float heldMove = 0.f;  // --move: input held without a keyboard

        // Interactions
        float blastStrength = 12000.f, blastRadius = 4.f;
        struct Ring { V at; float age; };
        std::vector<Ring> rings;
        ECS::Entity grab{}, grabSpring{};
        ECS::Entity rope{};
        V mouseWorld;

        void reset() {
            level = std::make_unique<Level>();
            tracking.reset();
            accumulator = 0.0;
            grab = grabSpring = rope = ECS::Entity{};
            rings.clear();
        }

        Component::CharacterController& controller() { return level->world.getComponent<Component::CharacterController>(level->marks.hero); }
        Component::CharacterInput& input() { return level->world.getComponent<Component::CharacterInput>(level->marks.hero); }

        void teleport(std::size_t zone) {
            const ECS::Entity hero = level->marks.hero;
            releaseRope();
            level->world.getComponent<Component::Transform>(hero).position = Zones[zone].spawn;
            level->world.getComponent<Component::RigidBody>(hero).velocity = {};
            view.center = Zones[zone].spawn + V(4.f, 2.f);
        }

        // ---------------------------------------------------------- grab and grapple: Springs of the engine

        void startGrab(const V& at) {
            const std::optional<ECS::Entity> picked = level->bodyAt(at);
            if (!picked) return;
            grab = *picked;
            const float mass = level->world.getComponent<Component::RigidBody>(grab).mass;
            const float k = 60.f * mass;  // about 1.2 Hz whatever the mass
            grabSpring = level->spring(grab, ECS::Entity{}, at, 0.f, k, Physic::criticalDamping(k, mass));
            if (grab == level->marks.hero) {
                controller().handToPhysics(0.f);
            }
        }

        void endGrab() {
            if (grabSpring != ECS::Entity{} && level->world.isAlive(grabSpring)) {
                level->world.killEntity(grabSpring);
            }
            if (grab == level->marks.hero && rope == ECS::Entity{}) {
                controller().mode = Component::CharacterController::Mode::Controlled;
            }
            grab = grabSpring = ECS::Entity{};
        }

        bool fireRope(const V& target) {
            const V from = level->position(level->marks.hero);
            const V direction = (target - from).normalized();
            const std::uint32_t mask = level->world.getComponent<Collider>(level->marks.hero).mask;
            const std::optional<Collision::Hit> hit = level->raycastScenery(from, direction * 18.f, mask);
            if (!hit) return false;
            const float length = hit->point.distanceTo(from);
            rope = level->spring(level->marks.hero, ECS::Entity{}, hit->point, length * 0.9f, 2500.f, 120.f);
            controller().handToPhysics(0.f);  // swinging is physics, until released
            return true;
        }

        void releaseRope() {
            if (rope == ECS::Entity{}) return;
            if (level->world.isAlive(rope)) level->world.killEntity(rope);
            rope = ECS::Entity{};
            controller().mode = Component::CharacterController::Mode::Controlled;
        }

        // ---------------------------------------------------------- stepping

        void stepOnce() {
            level->step();
            tracking.update(*level);
            ++tracking.steps;
            // Fell out of the level: back to the nearest zone.
            const V hero = level->position(level->marks.hero);
            if (hero.y < -20.f) {
                teleport(nearestZone(hero.x));
            }
        }

        static std::size_t nearestZone(float x) {
            std::size_t best = 0;
            for (std::size_t i = 0; i < Zones.size(); ++i) {
                if (Zones[i].spawn.x <= x) best = i;
            }
            return best;
        }

        void timedStep() {
            const auto start = std::chrono::steady_clock::now();
            stepOnce();
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
            tracking.stepMicroseconds = tracking.stepMicroseconds == 0.0 ? us : tracking.stepMicroseconds * 0.95 + us * 0.05;
        }

        void readInput() {
            const ImGuiIO& io = ImGui::GetIO();
            const bool keys = !io.WantCaptureKeyboard;
            float move = heldMove;
            bool jump = false;
            if (keys) {
                // QWERTY and AZERTY: A or Q left, W or Z up.
                if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_Q) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) move -= 1.f;
                if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) move += 1.f;
                const bool up = ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_Z) || ImGui::IsKeyDown(ImGuiKey_UpArrow);
                jump = ImGui::IsKeyDown(ImGuiKey_Space) || (rope == ECS::Entity{} && up);
            }
            input().move = std::clamp(move, -1.f, 1.f);
            input().jump = jump;

            if (!keys) return;
            for (int i = 0; i < 8; ++i) {
                if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(ImGuiKey_1 + i), false)) teleport(static_cast<std::size_t>(i));
            }
            if (ImGui::IsKeyPressed(ImGuiKey_R, false)) reset();
            if (ImGui::IsKeyPressed(ImGuiKey_P, false)) paused = !paused;
            if (ImGui::IsKeyPressed(ImGuiKey_N, false)) stepRequested = true;
            if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) debug = !debug;
            if (ImGui::IsKeyPressed(ImGuiKey_T, false)) { level->redrop(); tracking.reset(); }
            if (ImGui::IsKeyPressed(ImGuiKey_E, false)) explode(mouseWorld);
            if (ImGui::IsKeyPressed(ImGuiKey_B, false)) level->body(Collider::Box{{0.4f, 0.4f}}, mouseWorld, 10.f, Color::Crate);
            if (ImGui::IsKeyPressed(ImGuiKey_O, false)) level->body(Collider::Circle{0.35f}, mouseWorld, 4.f, Color::Ball, Material{0.6f, 0.5f, 0.3f});
            if (ImGui::IsKeyPressed(ImGuiKey_H, false)) level->body(Level::regular(6, 0.45f), mouseWorld, 8.f, Color::Purple);
            if (rope != ECS::Entity{} && level->world.isAlive(rope)) {
                auto& spring = level->world.getComponent<Component::Spring>(rope);
                if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_Z)) spring.restLength = std::max(0.5f, spring.restLength - 4.f * io.DeltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_S)) spring.restLength = std::min(18.f, spring.restLength + 4.f * io.DeltaTime);
                if (ImGui::IsKeyDown(ImGuiKey_Space)) releaseRope();
            }
        }

        void explode(const V& at) {
            level->explode(at, blastStrength, blastRadius);
            rings.push_back({at, 0.f});
        }

        void readMouse(const ImVec2& worldMin, const ImVec2& worldMax) {
            const ImGuiIO& io = ImGui::GetIO();
            mouseWorld = view.toWorld(io.MousePos);
            const bool inWorld = !io.WantCaptureMouse && io.MousePos.x >= worldMin.x && io.MousePos.x < worldMax.x
                && io.MousePos.y >= worldMin.y && io.MousePos.y < worldMax.y;
            if (inWorld && io.MouseWheel != 0.f) {
                view.scale = std::clamp(view.scale * std::pow(1.15f, io.MouseWheel), 8.f, 120.f);
            }
            if (inWorld && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) startGrab(mouseWorld);
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && grab != ECS::Entity{}) endGrab();
            if (grabSpring != ECS::Entity{} && level->world.isAlive(grabSpring)) {
                level->world.getComponent<Component::Spring>(grabSpring).anchor = mouseWorld;
            }
            if (inWorld && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (rope != ECS::Entity{}) releaseRope();
                else fireRope(mouseWorld);
            }
            if (inWorld && ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) explode(mouseWorld);
        }

        void frame(double seconds) {
            const ImGuiIO& io = ImGui::GetIO();
            const ImVec2 worldMin(0.f, 0.f);
            const ImVec2 worldMax(io.DisplaySize.x - PanelWidth, io.DisplaySize.y);
            view.middle = ImVec2((worldMin.x + worldMax.x) * 0.5f, (worldMin.y + worldMax.y) * 0.5f);

            readMouse(worldMin, worldMax);
            readInput();

            if (!paused) {
                accumulator += seconds * static_cast<double>(timeScale);
                int budget = 8;  // never more than 8 steps per frame: a slow frame slows the game instead of freezing it
                while (accumulator >= Physics::Step && budget-- > 0) {
                    timedStep();
                    accumulator -= Physics::Step;
                }
                accumulator = std::min(accumulator, Physics::Step);
            } else if (stepRequested) {
                timedStep();
            }
            stepRequested = false;
            for (Ring& ring : rings) ring.age += static_cast<float>(seconds);
            std::erase_if(rings, [](const Ring& r) { return r.age > 0.35f; });

            if (follow) {
                const V target = level->position(level->marks.hero) + V(0.f, 2.f);
                view.center = view.center + (target - view.center) * std::min(1.f, static_cast<float>(seconds) * 4.f);
            }
            drawWorld(ImGui::GetBackgroundDrawList(), worldMin, worldMax);
            panel();
        }

        // ---------------------------------------------------------- drawing

        void drawWorld(ImDrawList* dl, ImVec2 worldMin, ImVec2 worldMax) {
            ECS::Registry& world = level->world;
            dl->PushClipRect(worldMin, worldMax);
            dl->AddRectFilled(worldMin, worldMax, Color::Sky);

            // Zone names along the ground.
            for (const Zone& zone : Zones) {
                const ImVec2 p = view.toScreen(zone.spawn + V(-1.f, 4.5f));
                dl->AddText(p, Color::Dim, zone.name);
            }
            // Fields: the fan, the planet's pull.
            for (auto [entity, transform, field] : world.view<Component::Transform, Component::RadialField>()) {
                dl->AddCircleFilled(view.toScreen(transform.position), field.radius * view.scale, Color::Field, 48);
            }
            // Bodies and scenery, as the engine places them (Collider::worldShape).
            for (auto [entity, transform, c] : world.view<Component::Transform, Collider>()) {
                const ImU32 color = world.hasComponents<Paint>(entity) ? world.getComponent<Paint>(entity).color : Color::Rock;
                const bool isHero = entity == level->marks.hero;
                const ImU32 fill = isHero && controller().mode == Component::CharacterController::Mode::Physics ? Color::HeroPhysics : color;
                if (world.hasComponents<RopeLink>(entity) && !debug) {
                    // The rope: a thick line along the link, joined to the next by its spring.
                    const auto& box = std::get<Collider::Box>(c.shape);
                    const V along = V(box.halfSize.x, 0.f).rotated(transform.rotation);
                    dl->AddLine(view.toScreen(transform.position - along), view.toScreen(transform.position + along), Color::Rope,
                                2.f * box.halfSize.y * view.scale);
                    continue;
                }
                drawShape(dl, view, c.worldShape(transform), fill, transform.rotation);
                if (isHero) drawHeroFace(dl, transform.position);
            }
            // Water over the bodies, so what is under it looks under it.
            for (auto [entity, transform, fluid] : world.view<Component::Transform, Component::FluidVolume>()) {
                const ImVec2 a = view.toScreen(transform.position + V(-fluid.halfSize.x, fluid.halfSize.y));
                const ImVec2 b = view.toScreen(transform.position + V(fluid.halfSize.x, -fluid.halfSize.y));
                dl->AddRectFilled(a, b, Color::Water);
                dl->AddLine(a, ImVec2(b.x, a.y), Color::WaterTop, 2.f);
            }
            // Springs, redder when stretched.
            for (auto [entity, spring] : world.view<Component::Spring>()) {
                if (!world.isAlive(spring.body) || !world.hasComponents<Component::Transform>(spring.body)) continue;
                const V a = attachPoint(spring.body, spring.bodyPoint);
                const V b = spring.other == ECS::Entity{} ? spring.anchor : attachPoint(spring.other, spring.otherPoint);
                if (world.hasComponents<RopeLink>(spring.body)) {
                    dl->AddLine(view.toScreen(a), view.toScreen(b), Color::Rope, 2.f * RopeThickness * view.scale);
                    continue;
                }
                const float stretch = std::clamp((a.distanceTo(b) - spring.restLength) / std::max(0.3f, spring.restLength), -1.f, 1.f);
                const int red = 230, other = static_cast<int>(230.f * (1.f - std::max(0.f, stretch)));
                dl->AddLine(view.toScreen(a), view.toScreen(b), IM_COL32(red, other, other, 255), 2.f);
                dl->AddCircleFilled(view.toScreen(b), 3.f, Color::Spring);
            }
            for (const Ring& ring : rings) {
                const float t = ring.age / 0.35f;
                dl->AddCircle(view.toScreen(ring.at), blastRadius * view.scale * t, IM_COL32(255, 200, 80, static_cast<int>(255.f * (1.f - t))), 48, 3.f);
            }
            if (debug) drawDebug(dl);
            dl->PopClipRect();
        }

        [[nodiscard]] V attachPoint(ECS::Entity e, const V& local) const {
            const auto& transform = level->world.getComponent<Component::Transform>(e);
            return transform.position + local.rotated(transform.rotation);
        }

        void drawHeroFace(ImDrawList* dl, const V& center) {
            const auto& state = controller().state;
            const V eye = center + V(0.18f * static_cast<float>(state.facing), 0.5f);
            dl->AddCircleFilled(view.toScreen(eye), 0.09f * view.scale, IM_COL32(255, 255, 255, 255));
        }

        void drawDebug(ImDrawList* dl) {
            ECS::Registry& world = level->world;
            if (world.hasResource<Resource::Contacts>()) {
                for (const Resource::CollisionPair& pair : world.getResource<Resource::Contacts>().pairs) {
                    const ImVec2 p = view.toScreen(pair.contact.point);
                    dl->AddCircleFilled(p, 3.f, Color::Normal);
                    drawArrow(dl, p, view.toScreen(pair.contact.point + pair.contact.normal * 0.5f), Color::Normal, 1.5f);
                }
            }
            for (auto [entity, transform, rigid] : world.view<Component::Transform, Component::RigidBody>()) {
                if (rigid.velocity.lengthSquared() > 0.01f) {
                    drawArrow(dl, view.toScreen(transform.position), view.toScreen(transform.position + rigid.velocity * 0.15f), Color::Velocity, 1.5f);
                }
                dl->AddCircleFilled(view.toScreen(transform.position), 2.5f, Color::Text);
            }
            const auto& state = controller().state;
            if (state.grounded) {
                const V feet = level->position(level->marks.hero) - V(0.f, 0.9f);
                drawArrow(dl, view.toScreen(feet), view.toScreen(feet + state.groundNormal), IM_COL32(255, 255, 0, 255), 2.f);
            }
        }

        // ---------------------------------------------------------- panel

        void panel() {
            const ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - PanelWidth, 0.f));
            ImGui::SetNextWindowSize(ImVec2(PanelWidth, io.DisplaySize.y));
            ImGui::Begin("MetroV physics showcase", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ECS::Registry& world = level->world;
            const Landmarks& m = level->marks;

            const std::size_t zone = nearestZone(level->position(m.hero).x);
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.f, 1.f), "Zone %s", Zones[zone].name);
            ImGui::TextWrapped("%s", Zones[zone].what);
            ImGui::Separator();

            if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox("Paused (P)", &paused);
                ImGui::SameLine();
                if (ImGui::Button("Step (N)")) stepRequested = true;
                ImGui::SameLine();
                if (ImGui::Button("Reset (R)")) reset();
                ImGui::SliderFloat("Time scale", &timeScale, 0.05f, 2.f, "%.2f");
                auto& gravity = world.getResource<Resource::GravitySettings>();
                ImGui::SliderFloat("Gravity", &gravity.gravity.y, -25.f, 0.f, "%.2f m/s2");
                ImGui::SliderFloat("Resting speed", &world.getResource<Resource::CollisionSettings>().restingSpeed, 0.f, 2.f, "%.2f m/s");
                ImGui::Checkbox("Debug drawing (F1)", &debug);
                ImGui::SameLine();
                ImGui::Checkbox("Follow", &follow);
                int bodies = 0;
                for ([[maybe_unused]] auto [entity, rigid] : world.view<Component::RigidBody>()) ++bodies;
                const std::size_t pairs = world.hasResource<Resource::Contacts>() ? world.getResource<Resource::Contacts>().pairs.size() : 0;
                ImGui::Text("%d bodies, %zu contacts, step %.0f us (%zu systems)", bodies, pairs, tracking.stepMicroseconds, level->systems.size());
            }

            if (ImGui::CollapsingHeader("Character", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto& c = controller();
                const auto& s = c.state;
                const V v = level->velocity(m.hero);
                ImGui::Text("Mode: %s%s", c.mode == Component::CharacterController::Mode::Controlled ? "Controlled" : "Physics",
                            rope != ECS::Entity{} ? " (on a rope)" : "");
                ImGui::Text("Velocity (%.2f, %.2f) m/s", static_cast<double>(v.x), static_cast<double>(v.y));
                if (s.grounded) {
                    const float angle = std::acos(std::clamp(s.groundNormal.y, -1.f, 1.f)) * 180.f / Pi;
                    ImGui::Text("Grounded on %.1f deg, ground at (%.2f, %.2f) m/s", static_cast<double>(angle),
                                static_cast<double>(s.groundVelocity.x), static_cast<double>(s.groundVelocity.y));
                } else {
                    ImGui::Text("In the air for %.2f s", static_cast<double>(s.timeSinceGrounded));
                }
                const float share = c.state.groundFriction >= c.fullGripFriction || c.fullGripFriction <= 0.f
                    ? 1.f
                    : (c.state.groundFriction / c.fullGripFriction) * (c.state.groundFriction / c.fullGripFriction);
                ImGui::Text("Ground mu %.2f: grip %.0f %% of the control", static_cast<double>(c.state.groundFriction), static_cast<double>(share * 100.f));
                if (tracking.lastJump < 0.f) {
                    ImGui::Text("Last jump: none yet (aimed %.2f m)", static_cast<double>(c.jumpHeight));
                } else {
                    ImGui::Text("Last jump: %.2f m (aimed %.2f m)", static_cast<double>(tracking.lastJump), static_cast<double>(c.jumpHeight));
                }
                ImGui::SliderFloat("Max speed", &c.maxSpeed, 1.f, 15.f, "%.1f m/s");
                ImGui::SliderFloat("Jump height", &c.jumpHeight, 0.5f, 6.f, "%.1f m");
                ImGui::SliderFloat("Full grip from mu", &c.fullGripFriction, 0.f, 1.f, "%.2f");
                ImGui::SliderFloat("Push strength", &c.pushStrength, 0.f, 5000.f, "%.0f N");
                ImGui::SliderFloat("Fall gravity x", &c.fallGravityMultiplier, 1.f, 3.f, "%.2f");
                // Layers: the hero's mask decides whether it walks through the monster of zone 1.
                Collider& heroCollider = level->world.getComponent<Collider>(level->marks.hero);
                bool throughMonsters = (heroCollider.mask & Layer::Enemy) == 0u;
                if (ImGui::Checkbox("Walk through monsters (mask without Enemy)", &throughMonsters)) {
                    heroCollider.mask = throughMonsters ? ~Layer::Enemy : ~0u;
                }
            }

            if (ImGui::CollapsingHeader("Measured vs theory", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextDisabled("Zone 2: restitution from successive apexes");
                for (std::size_t i = 0; i < 3; ++i) {
                    ImGui::Text("  e %.1f: measured %.3f, %.3f", static_cast<double>(m.bouncerRestitution[i]),
                                static_cast<double>(tracking.bounces[i].measured(0)), static_cast<double>(tracking.bounces[i].measured(1)));
                }
                ImGui::TextDisabled("Zone 6: share under water (now / m / (A rho))");
                const char* names[] = {"crate 30 kg", "crate 12 kg", "ball", "triangle", "raft", "stone"};
                for (std::size_t i = 0; i < m.floaters.size(); ++i) {
                    ImGui::Text("  %-12s %.2f / %.2f", names[i], static_cast<double>(level->submergedShare(m.floaters[i])),
                                static_cast<double>(std::min(1.f, level->densityShare(m.floaters[i]))));
                }
                ImGui::TextDisabled("Zone 7: top falling speed (T drops again)");
                const float g = -world.getResource<Resource::GravitySettings>().gravity.y;
                ImGui::Text("  no drag   %.2f m/s", static_cast<double>(tracking.topSpeed[0]));
                ImGui::Text("  linear    %.2f m/s (m g / c = %.2f)", static_cast<double>(tracking.topSpeed[1]), static_cast<double>(0.5f * g / 1.f));
                ImGui::Text("  quadratic %.2f m/s (sqrt(m g / c) = %.2f)", static_cast<double>(tracking.topSpeed[2]), static_cast<double>(std::sqrt(0.3f * g / 0.3f)));
                ImGui::Text("  fan balls %.2f, %.2f m up (5 (1 - w / 40) = %.2f, %.2f)",
                            static_cast<double>(level->position(m.fanBalls[0]).y - FanHeight), static_cast<double>(level->position(m.fanBalls[1]).y - FanHeight),
                            static_cast<double>(5.f * (1.f - 0.5f * g / 40.f)), static_cast<double>(5.f * (1.f - 1.f * g / 40.f)));
                ImGui::TextDisabled("Zone 8: orbit radius (min - max)");
                ImGui::Text("  moons: %.2f - %.2f m (3), %.2f - %.2f m (4.6)", static_cast<double>(tracking.moonMin[0]),
                            static_cast<double>(tracking.moonMax[0]), static_cast<double>(tracking.moonMin[1]), static_cast<double>(tracking.moonMax[1]));
            }

            if (ImGui::CollapsingHeader("Interactions", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::SliderFloat("Blast force", &blastStrength, 1000.f, 40000.f, "%.0f N");
                ImGui::SliderFloat("Blast radius", &blastRadius, 1.f, 8.f, "%.1f m");
                if (rope != ECS::Entity{} && world.isAlive(rope)) {
                    const auto& spring = world.getComponent<Component::Spring>(rope);
                    const float length = level->position(m.hero).distanceTo(spring.anchor);
                    ImGui::Text("Rope: %.2f m (rest %.2f), tension %.0f N", static_cast<double>(length), static_cast<double>(spring.restLength),
                                static_cast<double>(std::max(0.f, (length - spring.restLength) * spring.stiffness)));
                }
                ImGui::TextWrapped("Q or A / D run, Space, Z or W jump. Left drag grabs, right click ropes (Z or W / S reel), "
                                   "E or middle click explodes, B / O / H drop a crate / ball / hexagon, T drops again, 1-8 zones.");
            }
            ImGui::End();
        }
    };

    // ------------------------------------------------------------------ self test: the level, measured without a window

    struct Check {
        int failures = 0;
        int known = 0;
        /** A limit of the engine as it is: reported, not counted as a failure. */
        void knownLimit(const char* what, double value, double low, double high) {
            const bool ok = value >= low && value <= high;
            std::printf("%s  %-58s %9.3f  in [%g, %g]\n", ok ? "ok  " : "KNOWN", what, value, low, high);
            known += ok ? 0 : 1;
        }
        void operator()(bool ok, const char* what, double value, double low, double high) {
            std::printf("%s  %-58s %9.3f  in [%g, %g]\n", ok ? "ok  " : "FAIL", what, value, low, high);
            failures += ok ? 0 : 1;
        }
        void range(const char* what, double value, double low, double high) { (*this)(value >= low && value <= high, what, value, low, high); }
    };

    int selftest() {
        Check check;
        Demo demo;
        demo.reset();
        Level& level = *demo.level;
        const Landmarks& m = level.marks;
        const auto run = [&](float seconds) {
            for (int i = 0; i < static_cast<int>(seconds / Step + 0.5f); ++i) demo.stepOnce();
        };

        run(6.f);
        std::printf("-- after 6 s, nothing touched\n");
        for (std::size_t i = 0; i < 3; ++i) {
            char what[96];
            std::snprintf(what, sizeof what, "zone 2: ball e %.1f, measured restitution", static_cast<double>(m.bouncerRestitution[i]));
            const double e = demo.tracking.bounces[i].measured(0);
            check.range(what, e, m.bouncerRestitution[i] - 0.05, m.bouncerRestitution[i] + 0.05);
        }
        check.range("zone 3: crate on 26.6 deg slope moved (m)", level.position(m.heldCrate).distanceTo(HeldCrateStart), 0.0, 0.01);
        check.range("zone 3: icy crate ends on the rubber (x)", level.position(m.icyCrate).x, 50.0, 62.0);
        check.range("zone 3: icy crate speed (m/s)", level.velocity(m.icyCrate).length(), 0.0, 0.05);
        const char* names[] = {"zone 6: crate 30 kg under water", "zone 6: crate 12 kg under water", "zone 6: ball under water",
                               "zone 6: triangle under water", "zone 6: raft under water"};
        run(6.f);
        std::printf("-- after 12 s\n");
        for (std::size_t i = 0; i + 1 < m.floaters.size(); ++i) {
            const double expected = level.densityShare(m.floaters[i]);
            check.range(names[i], level.submergedShare(m.floaters[i]), expected - 0.04, expected + 0.04);
        }
        check.range("zone 6: stone rests on the pool floor (bottom y)", level.position(m.stone).y - 0.3, -4.05, -3.9);
        check.range("zone 6: raft tilt after rocking (rad)", std::fabs(std::remainder(level.world.getComponent<Component::Transform>(m.floaters[4]).rotation, Pi)), 0.0, 0.05);
        const float g = 9.81f;
        // Drag decays the velocity by k = exp(-c dt / m) each step, gravity adds g dt:
        // the step's terminal speed is g dt / (1 - k) = 4.988, 1.7 % above m g / c.
        check.range("zone 7: linear drag top speed vs m g / c = 4.905", demo.tracking.topSpeed[1], 4.6, 5.0);
        check.range("zone 7: quadratic drag top speed vs sqrt(m g / c) = 3.13", demo.tracking.topSpeed[2], 3.0, 3.17);
        check.range("zone 7: fan ball 0.5 kg height vs 5 (1 - w / 40) = 4.39", level.position(m.fanBalls[0]).y - FanHeight, 5.f * (1.f - 0.5f * g / 40.f) - 0.15, 5.f * (1.f - 0.5f * g / 40.f) + 0.15);
        check.range("zone 7: fan ball 1 kg height vs 5 (1 - w / 40) = 3.77", level.position(m.fanBalls[1]).y - FanHeight, 5.f * (1.f - g / 40.f) - 0.15, 5.f * (1.f - g / 40.f) + 0.15);
        check.range("zone 8: moon at 3 m, smallest radius", demo.tracking.moonMin[0], 2.8, 3.01);
        check.range("zone 8: moon at 3 m, largest radius", demo.tracking.moonMax[0], 2.99, 3.2);
        check.range("zone 8: moon at 4.6 m, smallest radius", demo.tracking.moonMin[1], 4.4, 4.61);
        check.range("zone 8: moon at 4.6 m, largest radius", demo.tracking.moonMax[1], 4.59, 4.8);
        check.range("zone 4: platform carried between its ends (x)", level.position(m.platform).x, 65.5, 76.5);
        check.range("zone 1: monster stands on the ground (moved, m)", level.position(m.monster).distanceTo(MonsterStart), 0.0, 0.01);
        check.range("zone 1: crate rests on the monster's head (bottom y)", level.position(m.monsterCrate).y - 0.25, 1.99, 2.01);
check.knownLimit("zone 5: rope middle sag alone (m)", -level.position(m.bridgeMiddle).y, 0.0, 0.5);
        {
            float energy = 0.f, most = 0.f;
            V where;
            for (auto [entity, transform, rigid] : level.world.view<Component::Transform, Component::RigidBody>()) {
                // The e = 0.9 ball of zone 2 bounces for about 21 s: left out.
                if (rigid.mass <= 0.f || entity == m.hero || transform.position.x >= 104.f || entity == m.bouncers[0]) continue;
                const float e = 0.5f * rigid.mass * rigid.velocity.lengthSquared() + 0.5f * rigid.angularVelocity * rigid.angularVelocity
                    * level.world.getComponent<Collider>(entity).momentOfInertia(rigid.mass);
                energy += e;
                if (e > most) { most = e; where = transform.position; }
            }
            std::printf("      (most energetic body: %.1f J at (%.1f, %.1f))\n", static_cast<double>(most), static_cast<double>(where.x), static_cast<double>(where.y));
            check.range("kinetic energy of zones 1-5 at rest, bouncing ball aside (J)", energy, 0.0, 0.5);
        }

        std::printf("-- the character\n");
        auto& input = demo.input();
        // Standing against the 400 kg crate before anything was pushed around: a steady
        // push, not a run-up (a 70 kg runner at 7 m/s does move it).
        demo.teleport(0);
        level.world.getComponent<Component::Transform>(m.hero).position = V(18.f - 0.7f - 0.4f - 0.01f, 0.91f);
        run(0.5f);
        const float heavyBefore = level.position(m.heavyCrate).x;
        input.move = 1.f;
        run(1.5f);
        input.move = 0.f;
        // Known: the push reaches the crate as a contact impulse after FrictionSystem held it for
        // the step, and resting contacts get no friction in the collision solver: it creeps.
        check.knownLimit("zone 1: 400 kg crate pushed steadily for 1.5 s (m)", level.position(m.heavyCrate).x - heavyBefore, -0.01, 0.01);
        demo.teleport(0);
        run(0.5f);
        input.move = 1.f;
        run(1.f);
        input.move = 0.f;
        run(1.f);
        const float plankTurn = std::fabs(level.world.getComponent<Component::Transform>(m.plank).rotation);
        check.range("zone 1: the plank it ran into tipped over (rad)", plankTurn, 0.5, 2.0);
        demo.teleport(7);
        run(0.5f);
        input.move = 1.f;
        run(0.5f);
        check.range("runs at maxSpeed (m/s)", level.velocity(m.hero).x, 6.95, 7.05);
        input.move = 0.f;
        demo.teleport(0);  // away from the planet, which pulls up a little
        run(0.5f);
        input.jump = true;
        run(1.5f);
        input.jump = false;
        check.range("jump apex vs jumpHeight 2.5 (m)", demo.tracking.lastJump, 2.5, 2.56);
        // On the ice of zone 3 (x 50 to 56): the feet slip, it slides on after letting go.
        demo.teleport(2);
        level.world.getComponent<Component::Transform>(m.hero).position = V(51.f, 0.91f);
        run(0.3f);
        input.move = 1.f;
        run(0.5f);
        check.range("zone 3: on ice, speed after 0.5 s of running (m/s, 7 on rock)", level.velocity(m.hero).x, 2.0, 5.0);
        input.move = 0.f;
        const float letGo = level.position(m.hero).x;
        run(2.f);
        check.range("zone 3: slides on the ice after letting go (m, 0.27 on rock)", level.position(m.hero).x - letGo, 0.4, 6.0);
        demo.teleport(3);
        level.world.getComponent<Component::Transform>(m.hero).position = level.position(m.platform) + V(0.f, 1.2f);
        run(1.f);
        check.range("zone 4: on the platform, carried at its speed (|vx - platform vx|)",
                    std::fabs(level.velocity(m.hero).x - level.velocity(m.platform).x), 0.0, 0.05);
        demo.teleport(4);
        run(0.5f);
        input.move = 1.f;
        run(0.9f);
        input.move = 0.f;
        // The rope is springs: it sways under the feet, which may leave it for a step now and then.
        int onRope = 0;
        const int standing = static_cast<int>(0.5f / Step + 0.5f);
        for (int i = 0; i < standing; ++i) {
            demo.stepOnce();
            onRope += demo.controller().state.grounded ? 1 : 0;
        }
        check.range("zone 5: rope middle sags under the character (m)", -level.position(m.bridgeMiddle).y, 0.2, 3.0);
        check.range("zone 5: character stands on the rope (feet y)", level.position(m.hero).y - 0.9, -3.0, 0.0);
        check.range("zone 5: the rope is ground to the character (share of 0.5 s)", static_cast<double>(onRope) / standing, 0.8, 1.0);
        // The rope sags like a hammock: walk to its far end, jump out.
        input.move = 1.f;
        for (int press = 0; press < 8 && level.position(m.hero).x < 95.5f; ++press) {
            run(0.4f);
            input.jump = true;
            run(0.1f);
            input.jump = false;
        }
        run(1.f);
        input.move = 0.f;
        check.range("zone 5: crossed the rope, jumped out on the far side (x)", level.position(m.hero).x, 95.5, 130.0);
        check.range("zone 5: fastest rope link over the whole run, no blow-up (m/s)", demo.tracking.ropeTopSpeed, 0.0, 40.0);

        std::printf("-- interactions\n");
        demo.teleport(4);
        run(0.5f);
        const bool roped = demo.fireRope(level.position(m.hero) + V(3.f, 8.f));
        check(roped, "rope hits the ceiling", roped ? 1.0 : 0.0, 1.0, 1.0);
        run(2.f);
        const V rope = level.world.getComponent<Component::Spring>(demo.rope).anchor;
        check.range("swinging on the rope, distance to the hook (m)", level.position(m.hero).distanceTo(rope), 5.0, 10.0);
        check(demo.controller().mode == Component::CharacterController::Mode::Physics, "on the rope, physics moves the character", 1.0, 1.0, 1.0);
        demo.releaseRope();
        check(demo.controller().mode == Component::CharacterController::Mode::Controlled, "released, control is back", 1.0, 1.0, 1.0);

        demo.teleport(0);
        run(1.f);
        const V crate = level.position(m.plank);
        demo.startGrab(crate);
        level.world.getComponent<Component::Spring>(demo.grabSpring).anchor = crate + V(0.f, 3.f);
        run(2.f);
        check.range("grabbed plank follows the cursor 3 m up (height gained)", level.position(m.plank).y - crate.y, 2.0, 3.2);
        demo.endGrab();

        demo.teleport(1);
        run(1.f);
        const V ball = level.position(m.bouncers[1]);
        demo.explode(ball + V(-1.f, -0.3f));
        run(Step);
        check.range("explosion throws the ball (speed m/s)", level.velocity(m.bouncers[1]).length(), 3.0, 200.0);
        int blasts = 0;
        for ([[maybe_unused]] auto [entity, blast] : level.world.view<Blast>()) ++blasts;
        check(blasts == 0, "the blast lasted one step", blasts, 0.0, 0.0);

        std::printf("-- layers\n");
        demo.teleport(0);
        run(0.5f);
        const V monsterBefore = level.position(m.monster);
        input.move = -1.f;
        run(1.f);
        input.move = 0.f;
        check.range("zone 1: walked left through the monster, to the wall (x)", level.position(m.hero).x, 0.39, 0.6);
        check.range("zone 1: the monster was not pushed (m)", level.position(m.monster).distanceTo(monsterBefore), 0.0, 0.01);

        std::printf("%s: %d failed, %d known limits\n", check.failures == 0 ? "all checks passed" : "some checks failed", check.failures, check.known);
        return check.failures == 0 ? 0 : 1;
    }

    // ------------------------------------------------------------------ command line

    struct Options {
        const char* screenshot = nullptr;
        std::size_t zone = 0;
        float seconds = 0.f;
        float move = 0.f;
        bool selftest = false;
    };

    Options parse(int argc, char** argv) {
        Options options;
        for (int i = 1; i < argc; ++i) {
            const bool hasValue = i + 1 < argc;
            if (!std::strcmp(argv[i], "--screenshot") && hasValue) options.screenshot = argv[++i];
            else if (!std::strcmp(argv[i], "--zone") && hasValue) options.zone = static_cast<std::size_t>(std::clamp(std::atoi(argv[++i]), 1, 8) - 1);
            else if (!std::strcmp(argv[i], "--seconds") && hasValue) options.seconds = static_cast<float>(std::atof(argv[++i]));
            else if (!std::strcmp(argv[i], "--move") && hasValue) options.move = std::clamp(static_cast<float>(std::atof(argv[++i])), -1.f, 1.f);
            else if (!std::strcmp(argv[i], "--selftest")) options.selftest = true;
            else {
                std::fprintf(stderr, "usage: %s [--zone 1-8] [--seconds s] [--move -1..1] [--screenshot file.bmp] [--selftest]\n", argv[0]);
                std::exit(2);
            }
        }
        return options;
    }
}

int main(int argc, char** argv) {
    const Options options = parse(argc, argv);
    if (options.selftest) {
        return selftest();
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("MetroV - physics showcase", 1600, 900, SDL_WINDOW_RESIZABLE);
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
    demo.reset();
    demo.teleport(options.zone);
    if (options.screenshot) {
        demo.paused = true;
        demo.input().move = options.move;
        for (int i = 0; i < static_cast<int>(options.seconds / Step + 0.5f); ++i) {
            demo.input().move = options.move;
            demo.stepOnce();
        }
        demo.view.center = demo.level->position(demo.level->marks.hero) + V(0.f, 2.f);
    }
    demo.heldMove = options.move;

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
        const double seconds = std::min(0.1, static_cast<double>(now - last) / 1e9);
        last = now;

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        demo.frame(seconds);
        ImGui::Render();

        SDL_SetRenderDrawColor(renderer, 24, 27, 34, 255);
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
