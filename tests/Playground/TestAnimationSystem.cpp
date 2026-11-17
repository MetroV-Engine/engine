#include <boost/ut.hpp>

#include "Support.hpp"
#include "Systems/AnimationSystem/AnimationSystem.hpp"

using namespace boost::ut;
using namespace PlaygroundTest;

void run_animation_system_tests()
{
    "animation: idle clip when standing, walk when walking"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        AnimationSystem system;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).sheet == "idle.png");
        world.getComponent<PlayerController>(bull).walking = true;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).sheet == "walk.png");
        expect(world.getComponent<Sprite>(bull).frame == 2_i) << "walking resumes from the standing frame";
    };

    "animation: jump frames by vertical speed, then the landing frame"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        AnimationSystem system;
        PlayerController &controller = world.getComponent<PlayerController>(bull);
        controller.airborne = true;
        controller.velocityY = -100.0f;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).sheet == "jump.png");
        expect(world.getComponent<Sprite>(bull).frame == 0_i);
        world.getComponent<PlayerController>(bull).velocityY = 100.0f;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).frame == 1_i);
        world.getComponent<PlayerController>(bull).airborne = false;
        world.getComponent<PlayerController>(bull).landingLeft = 0.05f;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).frame == 2_i);
    };

    "animation: walk at 12 fps advances one frame per 1/12 s and wraps at 8"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        AnimationSystem system;
        world.getComponent<PlayerController>(bull).walking = true;
        system.update(world, 0.0);
        system.update(world, 1.0 / 12.0 + 1e-4);
        expect(world.getComponent<Sprite>(bull).frame == 3_i);
        system.update(world, 5.0 / 12.0 + 1e-4);
        expect(world.getComponent<Sprite>(bull).frame == 0_i) << "3 + 5 wraps to 0";
    };

    "animation: stopping restarts the idle loop at frame 0"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        AnimationSystem system;
        system.update(world, 3.0 / 6.0 + 1e-4);
        expect(world.getComponent<Sprite>(bull).frame == 3_i);
        world.getComponent<PlayerController>(bull).walking = true;
        system.update(world, 0.2);
        world.getComponent<PlayerController>(bull).walking = false;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).sheet == "idle.png");
        expect(world.getComponent<Sprite>(bull).frame == 0_i);
    };

    "animation: no idle sheet stands on walk frame 2"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        world.getComponent<Animator>(bull).idle.sheet.clear();
        AnimationSystem system;
        system.update(world, 1.0);
        expect(world.getComponent<Sprite>(bull).sheet == "walk.png");
        expect(world.getComponent<Sprite>(bull).frame == 2_i);
    };

    "animation: flipX follows the facing"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        AnimationSystem system;
        world.getComponent<PlayerController>(bull).facingLeft = true;
        system.update(world, 0.0);
        expect(world.getComponent<Sprite>(bull).flipX);
    };
}
