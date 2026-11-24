#include <boost/ut.hpp>

#include "Support.hpp"
#include "Systems/PlayerSystem/PlayerSystem.hpp"

using namespace boost::ut;
using namespace PlaygroundTest;

void run_player_system_tests()
{
    "player: walking right moves 300 u/s and faces right"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).right = true;
        system.update(world, 0.5);
        expect(world.getComponent<Transform>(bull).position.x == 270.0_f);
        expect(!world.getComponent<PlayerController>(bull).facingLeft);
        expect(world.getComponent<PlayerController>(bull).walking);
    };

    "player: walking left flips"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).left = true;
        system.update(world, 0.1);
        expect(world.getComponent<Transform>(bull).position.x == 90.0_f);
        expect(world.getComponent<PlayerController>(bull).facingLeft);
    };

    "player: no input stands still and stops walking"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).right = true;
        system.update(world, 0.1);
        input(world).right = false;
        system.update(world, 0.1);
        expect(world.getComponent<Transform>(bull).position.x == 150.0_f);
        expect(!world.getComponent<PlayerController>(bull).walking);
    };

    "player: jump is an edge and ignored mid-air"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).jumpPressed = true;
        system.update(world, 0.0);
        const PlayerController &controller = world.getComponent<PlayerController>(bull);
        expect(controller.airborne);
        expect(controller.velocityY == -800.0_f);
        system.update(world, 0.1);
        const float afterFall = world.getComponent<PlayerController>(bull).velocityY;
        system.update(world, 0.0);
        expect(world.getComponent<PlayerController>(bull).velocityY == afterFall) << "a press mid-air must not jump again";
    };

    "player: lands on groundY and shows the landing pose"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).jumpPressed = true;
        system.update(world, 0.0);
        input(world).jumpPressed = false;
        int frames = 0;
        while (world.getComponent<PlayerController>(bull).airborne && frames < 600) {
            system.update(world, 1.0 / 60.0);
            ++frames;
        }
        expect(frames > 30_i && frames < 45_i) << "about 0.6 s in the air";
        expect(world.getComponent<Transform>(bull).position.y == 600.0_f);
        expect(std::abs(world.getComponent<PlayerController>(bull).landingLeft - 0.1f) < 1e-5f);
    };

    "player: input is consumed, so a hidden Viewport cannot leave a key held"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = spawnBull(world);
        PlayerSystem system;
        input(world).right = true;
        input(world).jumpPressed = true;
        system.update(world, 0.1);
        expect(!input(world).right && !input(world).left && !input(world).jumpPressed);
        const float x = world.getComponent<Transform>(bull).position.x;
        system.update(world, 0.1);
        expect(world.getComponent<Transform>(bull).position.x == x) << "nothing refilled the input: the bull stops";
    };

    "player: no input resource means no input"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = world.spawnEntity("Bull");
        world.addComponent(bull, Transform{Vec2f{120.0f, 600.0f}, 0.0f});
        world.addComponent(bull, PlayerController{});
        PlayerSystem system;
        system.update(world, 0.5);
        expect(world.getComponent<Transform>(bull).position.x == 120.0_f);
    };
}
