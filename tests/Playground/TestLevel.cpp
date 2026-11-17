#include <boost/ut.hpp>

#include "Components/Camera.hpp"
#include "Components/PlayerController.hpp"
#include "Components/Sprite.hpp"
#include "Components/Transform.hpp"
#include "Level/Level/Level.hpp"

#include "ECS/ECS.hpp"

#include <optional>
#include <string>

using namespace boost::ut;

namespace {

std::optional<ECS::Entity> find(ECS::Registry &world, const std::string &name)
{
    for (auto [entity, entityName] : world.view<ECS::Name>())
        if (entityName.value == name)
            return entity;
    return std::nullopt;
}

int countStartingWith(ECS::Registry &world, const std::string &prefix)
{
    int count = 0;
    for (auto [entity, entityName] : world.view<ECS::Name>())
        count += entityName.value.starts_with(prefix) ? 1 : 0;
    return count;
}

} // namespace

void run_level_tests()
{
    "level: spawns the named entities"_test = [] {
        ECS::Registry world;
        spawnLevel(world);
        for (const char *name : {"Sky", "Sun", "Sun glow", "Far hills", "Near hills", "Ground", "Platform 1", "Platform 8",
                 "Bull", "Camera", "HUD bar", "HUD text"})
            expect(find(world, name).has_value()) << name;
        expect(countStartingWith(world, "Platform ") == 8_i);
        expect(countStartingWith(world, "Bush ") >= 4_i);
    };

    "level: the bull stands at (120, 600) and the camera follows it"_test = [] {
        ECS::Registry world;
        spawnLevel(world);
        const ECS::Entity bull = *find(world, "Bull");
        expect(world.getComponent<Transform>(bull).position.x == 120.0_f);
        expect(world.getComponent<Transform>(bull).position.y == 600.0_f);
        expect(world.hasComponents<Sprite, PlayerController>(bull));
        const ECS::Entity camera = *find(world, "Camera");
        expect(world.getComponent<Camera>(camera).followTarget == "Bull");
        expect(world.getComponent<Transform>(camera).position.y == 360.0_f);
    };
}
