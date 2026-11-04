// EcsSimulation: Play runs the systems, Stop puts the world back as it was.

#include <boost/ut.hpp>

#include <functional>
#include <string>
#include <utility>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/Logs.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "ecs/EcsSimulation/EcsSimulation.hpp"

using namespace boost::ut;

// Prefixed with "Sim": the TypeRegistry is process-wide.
namespace {
    struct SimPosition { float x = 0, y = 0; };
    struct SimTag {};
    struct SimNoCopy
    {
        SimNoCopy() = default;
        SimNoCopy(const SimNoCopy &) = delete;
        SimNoCopy &operator=(const SimNoCopy &) = delete;
        SimNoCopy(SimNoCopy &&) = default;
        SimNoCopy &operator=(SimNoCopy &&) = default;
        int value = 0;
    };

    // Runs a function on the world at every update, like a game system.
    class ScriptSystem : public ECS::ISystem
    {
    public:
        explicit ScriptSystem(std::function<void(ECS::Registry &, double)> script) : _script(std::move(script)) {}
        void update(ECS::Registry &world, double dt) override { _script(world, dt); }
        const std::string name() const override { return "ScriptSystem"; }

    private:
        std::function<void(ECS::Registry &, double)> _script;
    };

    // Counts the entities named name.
    int countNamed(ECS::Registry &world, const std::string &name)
    {
        int count = 0;
        for (auto [entity, entityName] : world.view<ECS::Name>()) {
            (void)entity;
            count += entityName.value == name;
        }
        return count;
    }
}

E_REFLECT(SimPosition) E_FIELD(x, float) E_FIELD(y, float) E_END
E_REFLECT(SimTag) E_END
E_REFLECT(SimNoCopy) E_FIELD(value, int) E_END

void run_ecs_simulation_tests()
{
    "Play runs the systems of the world with the frame time"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        int updates = 0;
        double lastDt = 0.0;
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &, double dt) {
            updates++;
            lastDt = dt;
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.update(0.032);
        simulation.end();
        expect(updates == 2_i);
        expect(lastDt == 0.032);
    };

    "Stop restores the values and names changed during play"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity e = world.spawnEntity("hero");
        world.addComponent(e, SimPosition{1, 2});
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            w.getComponent<SimPosition>(e).x += 10;
            w.getComponent<ECS::Name>(e).value = "changed";
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        expect(world.getComponent<SimPosition>(e).x == 11.0f);
        simulation.end();
        expect(world.isAlive(e)) << "same entity, same handle";
        expect(world.getComponent<SimPosition>(e).x == 1.0f && world.getComponent<SimPosition>(e).y == 2.0f);
        expect(world.getComponent<ECS::Name>(e).value == "hero");
    };

    "Stop removes the components added during play and gives back the removed ones"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity a = world.spawnEntity("a");
        const ECS::Entity b = world.spawnEntity("b");
        world.addComponent(b, SimTag{});
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            if (!w.hasComponents<SimTag>(a))
                w.addComponent(a, SimTag{});
            if (w.hasComponents<SimTag>(b))
                w.removeComponent<SimTag>(b);
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.end();
        expect(!world.hasComponents<SimTag>(a));
        expect(world.hasComponents<SimTag>(b));
    };

    "Stop kills the entities spawned during play"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.spawnEntity("kept");
        systems.addSystem<ScriptSystem>(world, [](ECS::Registry &w, double) {
            w.addComponent(w.spawnEntity("spawned"), SimPosition{});
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.update(0.016);
        expect(world.entityCount() == 3_ul);
        simulation.end();
        expect(world.entityCount() == 1_ul);
        expect(countNamed(world, "spawned") == 0_i);
    };

    "Stop spawns again the entities killed during play, with a new handle"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity victim = world.spawnEntity("victim");
        world.addComponent(victim, SimPosition{5, 5});
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            if (w.isAlive(victim))
                w.killEntity(victim);
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        expect(world.entityCount() == 0_ul);
        simulation.end();
        expect(world.entityCount() == 1_ul);
        expect(!world.isAlive(victim));
        int found = 0;
        for (auto [entity, name, position] : world.view<ECS::Name, SimPosition>()) {
            (void)entity;
            found += name.value == "victim" && position.x == 5.0f;
        }
        expect(found == 1_i);
    };

    "a component that cannot be copied keeps its play-time value, with a warning"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity e = world.spawnEntity("e");
        world.emplaceComponent<SimNoCopy>(e).value = 7;
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            w.getComponent<SimNoCopy>(e).value = 99;
        });
        Logger::instance().clear();
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.end();
        expect(world.hasComponents<SimNoCopy>(e)) << "not removed either";
        expect(world.getComponent<SimNoCopy>(e).value == 99_i);
        expect(logged(LogLevel::Warning, "SimNoCopy"));
    };

    "the editor's ids survive Stop, so the selection stays valid"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity kept = world.spawnEntity("kept");
        const ECS::Entity victim = world.spawnEntity("victim");
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            if (w.isAlive(victim))
                w.killEntity(victim);
        });
        EcsScene scene(world);
        const HierarchyModel::ObjectId keptId = scene.id(kept);
        const HierarchyModel::ObjectId victimId = scene.id(victim);
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.end();
        expect(scene.exists(keptId));
        expect(!scene.exists(victimId)) << "respawned with a new handle";
        expect(scene.roots().size() == 2_ul);
    };

    "each Play saves the world as it is now"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SimPosition{1, 0});
        systems.addSystem<ScriptSystem>(world, [&](ECS::Registry &w, double) {
            w.getComponent<SimPosition>(e).x = 100;
        });
        EcsSimulation simulation(world, systems);
        simulation.begin();
        simulation.update(0.016);
        simulation.end();
        world.getComponent<SimPosition>(e).x = 2; // edited between two plays
        simulation.begin();
        simulation.update(0.016);
        simulation.end();
        expect(world.getComponent<SimPosition>(e).x == 2.0f);
    };
}
