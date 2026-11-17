#include <boost/ut.hpp>

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"
#include "Systems/CameraFollowSystem/CameraFollowSystem.hpp"

#include "ECS/ECS.hpp"

using namespace boost::ut;

void run_camera_follow_system_tests()
{
    "camera: follows the named target's x and keeps its own y"_test = [] {
        ECS::Registry world;
        const ECS::Entity bull = world.spawnEntity("Bull");
        world.addComponent(bull, Transform{Vec2f{500.0f, 520.0f}, 0.0f});
        const ECS::Entity camera = world.spawnEntity("Camera");
        world.addComponent(camera, Transform{Vec2f{120.0f, 360.0f}, 0.0f});
        world.addComponent(camera, Camera{1.0f, "Bull"});
        CameraFollowSystem system;
        system.update(world, 0.016);
        expect(world.getComponent<Transform>(camera).position.x == 500.0_f);
        expect(world.getComponent<Transform>(camera).position.y == 360.0_f) << "a hop must not bob the camera";
    };

    "camera: unknown target leaves the camera where it is"_test = [] {
        ECS::Registry world;
        const ECS::Entity camera = world.spawnEntity("Camera");
        world.addComponent(camera, Transform{Vec2f{120.0f, 360.0f}, 0.0f});
        world.addComponent(camera, Camera{1.0f, "Nobody"});
        CameraFollowSystem system;
        system.update(world, 0.016);
        expect(world.getComponent<Transform>(camera).position.x == 120.0_f);
    };
}
