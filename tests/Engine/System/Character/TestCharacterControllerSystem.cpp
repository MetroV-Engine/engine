#include <boost/ut.hpp>

#include <algorithm>
#include <cmath>
#include <functional>

#include "ECS/ECS.hpp"
#include "Engine/Component/CharacterController.hpp"
#include "Engine/Component/CharacterInput.hpp"
#include "Engine/Component/Collider.hpp"
#include "Engine/Component/Drag.hpp"
#include "Engine/Component/SurfaceContact.hpp"
#include "Engine/System/Character/CharacterControllerSystem.hpp"
#include "Engine/System/Collision/CollisionResponseSystem.hpp"
#include "Engine/System/Collision/CollisionSystem.hpp"
#include "Engine/System/Drag/DragSystem.hpp"
#include "Engine/System/Friction/FrictionSystem.hpp"
#include "Engine/System/Gravity/GravitySystem.hpp"
#include "Engine/System/Integration/IntegrationSystem.hpp"
#include "../../Support/Approx.hpp"

using namespace boost::ut;
using namespace Engine;
using Component::CharacterController;
using Component::CharacterInput;
using Component::Collider;
using Component::RigidBody;
using Component::SurfaceContact;
using Component::Transform;
using System::CharacterControllerSystem;
using TestSupport::near;
using V = Math::Vector2f;

namespace {
constexpr float HalfHeight = 0.9f;    // the character is 0.8 m wide, 1.8 m tall
constexpr float Mass = 70.f;

Collider box(V halfSize) {
    Collider collider;
    collider.shape = Collider::Box{halfSize};
    return collider;
}

/** @brief A world with the whole physics step, in the documented order. */
struct Scene {
    ECS::Registry world;
    ECS::SystemManager systems;
    double step = 1.0 / 60.0;

    /** @brief withDrag: a DragSystem among the forces, for wind. */
    explicit Scene(bool withDrag = false) {
        systems.addSystem<Engine::System::GravitySystem>(world);
        if (withDrag) {
            systems.addSystem<Engine::System::DragSystem>(world);
        }
        systems.addSystem<CharacterControllerSystem>(world);
        systems.addSystem<Engine::System::FrictionSystem>(world);
        systems.addSystem<Engine::System::IntegrationSystem>(world);
        systems.addSystem<Engine::System::CollisionSystem>(world);
        systems.addSystem<Engine::System::CollisionResponseSystem>(world);
    }

    ECS::Entity spawnStatic(V position, V halfSize) {
        const ECS::Entity entity = world.spawnEntity();
        world.addComponent(entity, Transform{position});
        world.addComponent(entity, box(halfSize));
        return entity;
    }

    /** @brief Ground from x = -100 to 100, its top at y = 0. */
    ECS::Entity spawnGround() { return spawnStatic(V(0.f, -1.f), V(100.f, 1.f)); }

    ECS::Entity spawnBody(V position, V halfSize, float mass, V velocity = {}) {
        const ECS::Entity entity = spawnStatic(position, halfSize);
        world.addComponent(entity, RigidBody{mass, 1.f, velocity, {}});
        return entity;
    }

    /** @brief A character standing with its feet at `feet`. */
    ECS::Entity spawnCharacter(V feet = {0.f, 0.f}) {
        const ECS::Entity character = world.spawnEntity();
        world.addComponent(character, Transform{feet + V(0.f, HalfHeight)});
        Collider collider = box(V(0.4f, HalfHeight));
        collider.response = Collider::Response::Stop;
        world.addComponent(character, Collider{collider});
        world.addComponent(character, RigidBody{Mass});
        world.addComponent(character, CharacterController{});
        world.addComponent(character, CharacterInput{});
        return character;
    }

    void run(int steps, const std::function<void(int)>& eachStep = {}) {
        for (int index = 0; index < steps; ++index) {
            systems.update(world, step);
            if (eachStep) {
                eachStep(index);
            }
        }
    }

    CharacterInput& input(ECS::Entity character) { return world.getComponent<CharacterInput>(character); }
    CharacterController& controller(ECS::Entity character) { return world.getComponent<CharacterController>(character); }
    V& velocity(ECS::Entity entity) { return world.getComponent<RigidBody>(entity).velocity; }
    V& position(ECS::Entity entity) { return world.getComponent<Transform>(entity).position; }
    float feet(ECS::Entity character) { return position(character).y - HalfHeight; }

    /** @brief Presses jump for one step, then keeps it held or releases it. */
    void tapJump(ECS::Entity character, bool keepHeld = true) {
        input(character).jump = true;
        run(1);
        input(character).jump = keepHeld;
    }

    /** @brief Highest the feet get within the given number of steps. */
    float highestFeet(ECS::Entity character, int steps) {
        float highest = feet(character);
        run(steps, [&](int) { highest = std::max(highest, feet(character)); });
        return highest;
    }
};

void test_standing_and_running() {
    "a character on the ground stands still: grounded, no drift, landed once"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        int landings = 0;

        scene.run(120, [&](int) { landings += scene.controller(hero).state.landed ? 1 : 0; });

        expect(scene.controller(hero).state.grounded);
        expect(landings == 1_i);
        expect(near(scene.position(hero), V(0.f, HalfHeight), 1e-4f));
        expect(scene.velocity(hero).length() < 1e-4f);
    };

    "running reaches exactly maxSpeed after maxSpeed / acceleration, despite the ground's friction"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(5);
        scene.input(hero).move = 1.f;

        // 7 m/s at 70 m/s²: 0.1 s, 6 steps of 1/60 s.
        scene.run(5);
        expect(scene.velocity(hero).x < 7.f - 0.1f);
        scene.run(1);
        expect(near(scene.velocity(hero).x, 7.f, 1e-3f));
        scene.run(60);
        expect(near(scene.velocity(hero).x, 7.f, 1e-3f));
        expect(near(scene.feet(hero), 0.f, 1e-4f));
        expect(scene.controller(hero).state.facing == 1_i);
    };

    "a half-tilted stick walks at half the speed"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.input(hero).move = -0.5f;

        scene.run(60);

        expect(near(scene.velocity(hero).x, -3.5f, 1e-3f));
        expect(scene.controller(hero).state.facing == -1_i);
    };

    "releasing the input stops the character within maxSpeed / deceleration, without moving back"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.input(hero).move = 1.f;
        scene.run(30);
        scene.input(hero).move = 0.f;
        float slowest = 10.f;

        // 7 m/s at 90 m/s²: 0.078 s, 5 steps.
        scene.run(5, [&](int) { slowest = std::min(slowest, scene.velocity(hero).x); });

        expect(near(scene.velocity(hero).x, 0.f, 1e-5f));
        expect(slowest >= 0.f);
    };
}

void test_jump() {
    "the feet rise exactly jumpHeight, at 30, 60 and 144 steps per second and under double gravity"_test = [] {
        for (const double rate : {30.0, 60.0, 144.0}) {
            for (const float gravityScale : {1.f, 2.f}) {
                Scene scene;
                scene.step = 1.0 / rate;
                scene.spawnGround();
                const ECS::Entity hero = scene.spawnCharacter();
                scene.world.getComponent<RigidBody>(hero).gravityScale = gravityScale;
                scene.run(static_cast<int>(rate / 4));

                scene.tapJump(hero);
                const float highest = scene.highestFeet(hero, static_cast<int>(rate * 2));

                // Never below, at most g dt² / 8 above (Physic::speedToReachHeightForStep).
                const float dt = static_cast<float>(scene.step);
                const float bound = 9.81f * gravityScale * dt * dt / 8.f;
                expect(highest >= 2.5f - 1e-4f && highest <= 2.5f + bound + 1e-4f)
                    << "rate" << rate << "gravity scale" << gravityScale << "apex" << highest;
                expect(scene.controller(hero).state.grounded) << "back on the ground";
            }
        }
    };

    "holding jump jumps once: the next jump needs a new press"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        int jumps = 0;
        scene.input(hero).jump = true;

        scene.run(240, [&](int) { jumps += scene.controller(hero).state.jumped ? 1 : 0; });

        expect(jumps == 1_i);
        expect(scene.controller(hero).state.grounded);
    };

    "no second jump in the air"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.tapJump(hero, false);
        scene.run(1);
        int jumps = 0;

        // Right after the jump, within coyoteTime of leaving the ground: still no jump.
        for (int press = 0; press < 3; ++press) {
            scene.tapJump(hero, false);
            jumps += scene.controller(hero).state.jumped ? 1 : 0;
            scene.run(2);
        }

        expect(jumps == 0_i);
    };

    "releasing jump right away cuts the jump: a short hop"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);

        scene.tapJump(hero, false);
        const float highest = scene.highestFeet(hero, 120);

        // Half the speed, a quarter of the height (minus the first step, taken at full speed).
        expect(highest > 0.6f && highest < 0.8f) << "apex" << highest;
    };

    "the fall is quicker than the rise: falling gravity is fallGravityMultiplier times stronger"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.tapJump(hero);
        int rising = 0;
        int falling = 0;

        scene.run(180, [&](int) {
            if (!scene.controller(hero).state.grounded) {
                (scene.velocity(hero).y > 0.f ? rising : falling) += 1;
            }
        });

        // sqrt(1.5) less time to fall the same height.
        expect(near(static_cast<float>(rising) / static_cast<float>(falling), std::sqrt(1.5f), 0.1f))
            << rising << "steps up" << falling << "down";
    };

    "the fall speed never exceeds maxFallSpeed"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter(V(0.f, 200.f));
        float fastest = 0.f;

        scene.run(600, [&](int) { fastest = std::min(fastest, scene.velocity(hero).y); });

        expect(near(fastest, -25.f, 1e-3f));
        expect(near(scene.feet(hero), 0.f, 1e-3f)) << "landed on the ground, not through it";
    };
}

void test_forgiveness() {
    "coyote time: a jump just after walking off a ledge still works, a late one does not"_test = [] {
        for (const int stepsLate : {3, 12}) {
            Scene scene;
            scene.spawnStatic(V(-50.f, -1.f), V(50.f, 1.f));    // ground ending at x = 0
            const ECS::Entity hero = scene.spawnCharacter(V(-2.f, 0.f));
            scene.run(10);
            scene.input(hero).move = 1.f;
            int airborne = 0;
            while (airborne < stepsLate) {
                scene.run(1);
                airborne = scene.controller(hero).state.grounded ? 0 : airborne + 1;
            }

            const float jumpedFrom = scene.feet(hero);
            scene.tapJump(hero);

            expect(scene.controller(hero).state.jumped == (stepsLate == 3)) << "steps late" << stepsLate;
            if (stepsLate == 3) {
                // Already falling when it jumps: the jump replaces the fall, full height.
                expect(scene.highestFeet(hero, 120) - jumpedFrom >= 2.5f - 1e-3f);
            }
        }
    };

    "jump buffer: a press just before landing jumps on landing, an early one does not"_test = [] {
        for (const float pressedAbove : {0.15f, 1.5f}) {
            Scene scene;
            scene.spawnGround();
            const ECS::Entity hero = scene.spawnCharacter(V(0.f, 4.f));
            while (scene.feet(hero) > pressedAbove) {
                scene.run(1);
            }
            scene.tapJump(hero);
            int jumps = 0;

            scene.run(30, [&](int) { jumps += scene.controller(hero).state.jumped ? 1 : 0; });

            expect(jumps == (pressedAbove < 1.f ? 1 : 0)) << "pressed" << pressedAbove << "m above";
        }
    };

    "a wall is not ground: no jump from it"_test = [] {
        Scene scene;
        scene.spawnStatic(V(1.f, 10.f), V(0.5f, 10.f));    // a wall from x = 0.5
        const ECS::Entity hero = scene.spawnCharacter(V(0.f, 10.f));
        scene.input(hero).move = 1.f;
        scene.run(20);
        expect(fatal(scene.world.hasComponents<SurfaceContact>(hero))) << "pressed against the wall";

        scene.tapJump(hero);

        expect(!scene.controller(hero).state.grounded);
        expect(!scene.controller(hero).state.jumped);
        expect(scene.velocity(hero).y < 0.f);
    };
}

void test_air_and_wind() {
    "in the air the character steers with airAcceleration, and keeps its momentum with no input"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.tapJump(hero);
        scene.input(hero).move = 1.f;

        scene.run(6);    // 0.1 s at 40 m/s²

        expect(!scene.controller(hero).state.grounded);
        expect(near(scene.velocity(hero).x, 4.f, 1e-3f));

        scene.input(hero).move = 0.f;
        scene.run(6);    // 0.1 s at 5 m/s²

        expect(near(scene.velocity(hero).x, 3.5f, 1e-3f));
    };

    "a wind within its deceleration does not move a character standing still"_test = [] {
        Scene scene(true);
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        // Linear drag towards a 10 m/s wind to the left: 200 * 10 = 2000 N,
        // 28.6 m/s² on 70 kg, within the 90 m/s² it brakes with.
        scene.world.addComponent(hero, Component::Drag{200.f, 0.f, V(-10.f, 0.f)});

        scene.run(60);

        expect(near(scene.position(hero).x, 0.f, 1e-4f));
        expect(near(scene.velocity(hero).x, 0.f, 1e-4f));
    };

    "a wind stronger than the character's deceleration pushes it back"_test = [] {
        Scene scene(true);
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        // 1000 * 10 = 10000 N at rest, 143 m/s² on 70 kg, above the 90 m/s² it brakes with.
        scene.world.addComponent(hero, Component::Drag{1000.f, 0.f, V(-10.f, 0.f)});

        scene.run(30);

        expect(scene.position(hero).x < -0.1f);
        expect(scene.velocity(hero).x < 0.f);
    };
}

/**
 * @brief A slope rising to the right at `degrees`, 40 m wide, its surface the
 *        line y = x tan(angle); the character stands on it at x = 0 (its
 *        bottom-right corner on the surface).
 */
struct SlopeCharacter {
    Scene scene;
    ECS::Entity hero;
    float angle;

    explicit SlopeCharacter(float degrees) : angle(degrees * 3.14159265f / 180.f) {
        const float rise = std::tan(angle);
        const ECS::Entity slope = scene.world.spawnEntity();
        scene.world.addComponent(slope, Transform{V(0.f, 0.f)});
        Collider collider;
        collider.shape = Collider::Polygon::slope(V(20.f, 20.f * rise));
        scene.world.addComponent(slope, Collider{collider});
        hero = scene.spawnCharacter(V(0.f, rise * 0.4f));
    }

    [[nodiscard]] V along() const { return V(std::cos(angle), std::sin(angle)); }
};

void test_slopes() {
    "on a 30° slope the character stands still: no sliding"_test = [] {
        SlopeCharacter slope(30.f);
        slope.scene.run(10);
        const V start = slope.scene.position(slope.hero);

        slope.scene.run(120);

        expect(slope.scene.controller(slope.hero).state.grounded);
        expect(near(slope.scene.position(slope.hero), start, 1e-3f));
        expect(near(slope.scene.controller(slope.hero).state.groundNormal, V(-0.5f, std::sqrt(0.75f)), 1e-5f));
    };

    "running up a 30° slope: maxSpeed along the slope, on the ground all the way"_test = [] {
        SlopeCharacter slope(30.f);
        slope.scene.run(10);
        slope.scene.input(slope.hero).move = 1.f;
        int airborne = 0;

        slope.scene.run(60, [&](int) { airborne += slope.scene.controller(slope.hero).state.grounded ? 0 : 1; });

        expect(airborne == 0_i);
        expect(near(slope.scene.velocity(slope.hero), slope.along() * 7.f, 1e-3f));
    };

    "running down a 30° slope: maxSpeed along it, it stays on the ground instead of flying off"_test = [] {
        SlopeCharacter slope(30.f);
        slope.scene.run(10);
        slope.scene.input(slope.hero).move = -1.f;
        int airborne = 0;

        slope.scene.run(60, [&](int) { airborne += slope.scene.controller(slope.hero).state.grounded ? 0 : 1; });

        expect(airborne == 0_i);
        expect(near(slope.scene.velocity(slope.hero), slope.along() * -7.f, 1e-3f));
    };

    "jumping from a slope: the feet rise exactly jumpHeight"_test = [] {
        SlopeCharacter slope(30.f);
        slope.scene.run(10);
        const float feet = slope.scene.feet(slope.hero);

        slope.scene.tapJump(slope.hero);
        const float highest = slope.scene.highestFeet(slope.hero, 120);

        expect(highest - feet >= 2.5f - 1e-3f && highest - feet <= 2.5f + 1e-3f) << highest - feet;
    };

    "a 60° slope is steeper than maxGroundAngle: a wall, the character slides down it and cannot jump"_test = [] {
        SlopeCharacter slope(60.f);
        slope.scene.run(30);
        const V start = slope.scene.position(slope.hero);

        slope.scene.tapJump(slope.hero);

        expect(!slope.scene.controller(slope.hero).state.grounded);
        expect(!slope.scene.controller(slope.hero).state.jumped);
        slope.scene.run(30);
        expect(slope.scene.position(slope.hero).y < start.y - 0.5f) << "slid down";
    };
}

/**
 * @brief Flat ground (top y = 0) up to x = 0, a 30° slope from (0, 0) to
 *        (10, 5.77), then a plateau at that height.
 */
struct Hill {
    Scene scene;
    ECS::Entity hero;
    float top = 10.f * std::tan(30.f * 3.14159265f / 180.f);

    explicit Hill(V feet) {
        scene.spawnStatic(V(-20.f, -1.f), V(20.f, 1.f));
        const ECS::Entity slope = scene.world.spawnEntity();
        scene.world.addComponent(slope, Transform{V(5.f, top / 2.f)});
        Collider collider;
        collider.shape = Collider::Polygon::slope(V(5.f, top / 2.f));
        scene.world.addComponent(slope, Collider{collider});
        scene.spawnStatic(V(30.f, top - 1.f), V(20.f, 1.f));
        hero = scene.spawnCharacter(feet);
        scene.run(10);
    }

    int airborneSteps(int steps) {
        int airborne = 0;
        scene.run(steps, [&](int) { airborne += scene.controller(hero).state.grounded ? 0 : 1; });
        return airborne;
    }
};

void test_junctions() {
    "from flat ground up the slope onto the plateau: on the ground all the way, not launched at the top"_test = [] {
        Hill hill(V(-3.f, 0.f));
        hill.scene.input(hill.hero).move = 1.f;

        expect(hill.airborneSteps(150) == 0_i);
        expect(hill.scene.position(hill.hero).x > 11.f) << "on the plateau";
        expect(near(hill.scene.feet(hill.hero), hill.top, 1e-3f));
        expect(near(hill.scene.velocity(hill.hero), V(7.f, 0.f), 1e-3f));
    };

    "walking slowly over the top of the slope: on the ground all the way"_test = [] {
        for (const float move : {0.1f, 0.3f, 0.6f}) {
            Hill hill(V(7.f, 7.f * std::tan(30.f * 3.14159265f / 180.f)));
            hill.scene.input(hill.hero).move = move;

            // 4 m at move * 7 m/s, and a second more.
            const int steps = static_cast<int>(4.f / (move * 7.f) * 60.f) + 60;
            expect(hill.airborneSteps(steps) == 0_i) << "move" << move;
            expect(hill.scene.position(hill.hero).x > 10.5f) << "move" << move;
        }
    };

    "from the plateau down the slope onto flat ground: on the ground all the way, not flying off the top"_test = [] {
        Hill hill(V(14.f, 10.f * std::tan(30.f * 3.14159265f / 180.f)));
        hill.scene.input(hill.hero).move = -1.f;

        expect(hill.airborneSteps(150) == 0_i);
        expect(hill.scene.position(hill.hero).x < -1.f) << "back on the flat ground";
        expect(near(hill.scene.feet(hill.hero), 0.f, 1e-3f));
    };

    "a jump at the top of the slope is a jump: no snap back to the ground"_test = [] {
        Hill hill(V(-3.f, 0.f));
        hill.scene.input(hill.hero).move = 1.f;
        while (hill.scene.position(hill.hero).x < 9.5f) {
            hill.scene.run(1);
        }
        const float feet = hill.scene.feet(hill.hero);

        hill.scene.tapJump(hill.hero);
        const float highest = hill.scene.highestFeet(hill.hero, 60);

        expect(highest - feet > 2.4f);
    };

    "a launch the game gives on flat ground (a bounce pad) is kept: no sticking, no snapping back"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.velocity(hero) = V(2.f, 6.f);

        const float highest = scene.highestFeet(hero, 60);

        // 6 m/s up: about 6² / (2 g) = 1.8 m.
        expect(highest > 1.5f) << highest;
    };

    "a step deeper than groundSnapDistance is a fall"_test = [] {
        Scene scene;
        scene.spawnStatic(V(-20.f, -1.f), V(20.f, 1.f));       // top at y = 0, up to x = 0
        scene.spawnStatic(V(20.f, -2.f), V(20.f, 1.f));        // one meter lower beyond
        const ECS::Entity hero = scene.spawnCharacter(V(-1.f, 0.f));
        scene.run(10);
        scene.input(hero).move = 1.f;
        int airborne = 0;

        scene.run(60, [&](int) { airborne += scene.controller(hero).state.grounded ? 0 : 1; });

        expect(airborne > 5_i);
        expect(near(scene.feet(hero), -1.f, 1e-3f));
    };
}

void test_world() {
    "a moving platform carries the character, who runs relative to it"_test = [] {
        Scene scene;
        const ECS::Entity platform = scene.spawnBody(V(0.f, -0.25f), V(20.f, 0.25f), 0.f, V(3.f, 0.f));
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(60);

        expect(near(scene.velocity(hero).x, 3.f, 1e-3f));
        expect(near(scene.controller(hero).state.groundVelocity, V(3.f, 0.f), 1e-6f));

        scene.input(hero).move = 1.f;
        scene.run(30);

        expect(near(scene.velocity(hero).x, 10.f, 1e-3f));
        expect(near(scene.velocity(platform).x, 3.f, 1e-6f));
    };

    "pushing crates: a light one goes almost as fast, a heavy one much slower, a huge one not at all"_test = [] {
        float travelled[3] = {};
        const float masses[3] = {10.f, 100.f, 1000.f};
        for (int index = 0; index < 3; ++index) {
            Scene scene;
            scene.spawnGround();
            const ECS::Entity hero = scene.spawnCharacter(V(0.f, 0.f));
            const ECS::Entity crate = scene.spawnBody(V(0.9f, 0.5f), V(0.5f, 0.5f), masses[index]);
            scene.run(10);
            const float start = scene.position(crate).x;
            scene.input(hero).move = 1.f;

            scene.run(60);

            travelled[index] = scene.position(crate).x - start;
            expect(scene.position(crate).x - scene.position(hero).x >= 0.9f - 1e-3f) << "never inside the crate";
        }

        // pushStrength 1000 N, wood on wood (mu 0.3 sliding, 0.5 at rest).
        // 10 kg: (1000 - 29) N on 80 kg, 12 m/s², full speed in 0.6 s.
        expect(travelled[0] > 3.5f) << travelled[0];
        // 100 kg: (1000 - 294) N on 170 kg, 4.2 m/s²: about 2.1 m in 1 s.
        expect(near(travelled[1], 2.1f, 0.3f)) << travelled[1];
        // 1000 kg: static friction holds up to 4900 N.
        expect(travelled[2] == 0.f) << travelled[2];
    };
}

void test_physics_mode() {
    "handToPhysics: a knockback is not cancelled by the input, and control comes back after its duration"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.input(hero).move = 1.f;
        scene.velocity(hero) = V(-8.f, 4.f);
        scene.controller(hero).handToPhysics(0.25f);

        scene.run(6);    // 0.1 s in the air, flying left against the input

        expect(scene.controller(hero).mode == CharacterController::Mode::Physics);
        // Only the ground's friction during the first step, before it leaves the ground.
        expect(near(scene.velocity(hero).x, -8.f, 0.3f * 9.81f / 60.f + 1e-4f));

        scene.run(10);

        expect(scene.controller(hero).mode == CharacterController::Mode::Controlled);
        expect(scene.velocity(hero).x > -8.f + 0.5f) << "steering again";
    };

    "handToPhysics with no duration stays in Physics mode; friction then stops the character"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        scene.input(hero).move = 1.f;
        scene.controller(hero).handToPhysics(0.f);
        scene.velocity(hero) = V(-3.f, 0.f);

        scene.run(120);

        expect(scene.controller(hero).mode == CharacterController::Mode::Physics);
        expect(scene.velocity(hero).length() < 1e-4f);
        // Slid v² / (2 mu g) with the default friction 0.3: about 1.53 m, not run right.
        expect(near(scene.position(hero).x, -9.f / (2.f * 0.3f * 9.81f), 0.1f));
    };

    "in Physics mode the fall speed is still capped"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter(V(0.f, 200.f));
        scene.controller(hero).handToPhysics(0.f);
        float fastest = 0.f;

        scene.run(240, [&](int) { fastest = std::min(fastest, scene.velocity(hero).y); });

        expect(near(fastest, -25.f, 1e-3f));
    };

    "an off-center hit pushes the character but never tips it over"_test = [] {
        Scene scene;
        scene.spawnGround();
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(10);
        // A heavy ball hitting the top of its head from the side.
        const ECS::Entity rock = scene.world.spawnEntity();
        scene.world.addComponent(rock, Transform{V(-0.6f, 1.7f)});
        Collider ball;
        ball.shape = Collider::Circle{0.25f};
        scene.world.addComponent(rock, Collider{ball});
        scene.world.addComponent(rock, RigidBody{20.f, 0.f, V(8.f, 0.f), {}});

        scene.run(30);

        expect(scene.world.getComponent<Transform>(hero).rotation == 0.f);
        expect(scene.world.getComponent<RigidBody>(hero).angularVelocity == 0.f);
        expect(scene.world.getComponent<RigidBody>(hero).fixedRotation);
    };

    "a kinematic body with a controller is left alone"_test = [] {
        Scene scene;
        const ECS::Entity hero = scene.spawnCharacter(V(0.f, 5.f));
        scene.world.getComponent<RigidBody>(hero).mass = 0.f;
        scene.input(hero).move = 1.f;

        scene.run(10);

        expect(scene.velocity(hero) == V::zero());
        expect(scene.systems.getSystem<CharacterControllerSystem>()->name() == "CharacterControllerSystem");
    };
}
}

void test_slippery_ground() {
    // The character's own collider has the default muStatic 0.5: on a ground of muStatic
    // mu, the feet grip with sqrt(0.5 mu) (Physic::combinedFriction).
    const auto groundOf = [](Scene& scene, float staticFriction) {
        const ECS::Entity ground = scene.spawnGround();
        Collider& collider = scene.world.getComponent<Collider>(ground);
        collider.staticFriction = staticFriction;
        collider.kineticFriction = staticFriction * 0.5f;
        return ground;
    };

    "on ice the feet slip: the character speeds up at (mu / fullGripFriction)² of its acceleration"_test = [=] {
        Scene scene;
        groundOf(scene, 0.03f);
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(5);
        const float mu = std::sqrt(0.5f * 0.03f);
        expect(near(scene.controller(hero).state.groundFriction, mu, 1e-6f));

        scene.input(hero).move = 1.f;
        scene.run(30);

        // 70 m/s² * (0.1225 / 0.4)² = 6.56 m/s² for 0.5 s.
        const float grip = (mu / 0.4f) * (mu / 0.4f);
        expect(near(scene.velocity(hero).x, 70.f * grip * 0.5f, 1e-3f)) << scene.velocity(hero).x;
        expect(scene.controller(hero).state.grounded);
    };

    "on ice the character slides on after it stops running, about ten times farther than on wood"_test = [=] {
        const auto slide = [&](float staticFriction) {
            Scene scene;
            groundOf(scene, staticFriction);
            const ECS::Entity hero = scene.spawnCharacter();
            scene.input(hero).move = 1.f;
            scene.run(240);
            expect(near(scene.velocity(hero).x, 7.f, 1e-3f));
            scene.input(hero).move = 0.f;
            const float start = scene.position(hero).x;
            float slowest = 10.f;
            scene.run(240, [&](int) { slowest = std::min(slowest, scene.velocity(hero).x); });
            expect(near(scene.velocity(hero).x, 0.f, 1e-5f));
            expect(slowest >= 0.f) << "never moves back";
            return scene.position(hero).x - start;
        };
        const float onIce = slide(0.03f);
        const float onWood = slide(0.5f);

        // v² / (2 a), with a = 90 * (sqrt(0.015) / 0.4)² = 8.44 m/s² on ice, 90 on wood.
        const float iceDeceleration = 90.f * (0.5f * 0.03f) / (0.4f * 0.4f);
        expect(near(onIce, 49.f / (2.f * iceDeceleration), 0.1f)) << onIce;
        expect(near(onWood, 49.f / 180.f, 0.1f)) << onWood;
        expect(onIce > 8.f * onWood);
    };

    "a ground gripping at fullGripFriction gives full control, and so does a ground of fullGripFriction 0"_test = [=] {
        // sqrt(0.5 * 0.32) = 0.4: exactly fullGripFriction.
        Scene scene;
        groundOf(scene, 0.32f);
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(5);
        scene.input(hero).move = 1.f;
        scene.run(6);
        expect(near(scene.velocity(hero).x, 7.f, 1e-3f));

        Scene noLimit;
        groundOf(noLimit, 0.03f);
        const ECS::Entity other = noLimit.spawnCharacter();
        noLimit.controller(other).fullGripFriction = 0.f;
        noLimit.run(5);
        noLimit.input(other).move = 1.f;
        noLimit.run(6);
        expect(near(noLimit.velocity(other).x, 7.f, 1e-3f));
    };

    "the air does not depend on the last ground: airAcceleration is whole after leaving ice"_test = [=] {
        Scene scene;
        groundOf(scene, 0.03f);
        const ECS::Entity hero = scene.spawnCharacter();
        scene.run(5);
        scene.tapJump(hero);
        scene.input(hero).move = 1.f;
        scene.run(10);

        // 40 m/s² for 10 steps, the first one maybe still on the ground.
        expect(scene.velocity(hero).x > 40.f * 9.f / 60.f - 1e-3f) << scene.velocity(hero).x;
    };
}

void run_character_controller_system_tests() {
    test_slippery_ground();
    test_standing_and_running();
    test_jump();
    test_forgiveness();
    test_air_and_wind();
    test_slopes();
    test_junctions();
    test_world();
    test_physics_mode();
}
