// EcsBackend: what the Editor receives for an ECS world.

#include <boost/ut.hpp>

#include <string>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Support/Logs.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"
#include "scene/Selection.hpp"

using namespace boost::ut;

// Prefixed with "Backend": the TypeRegistry is process-wide.
namespace {
    struct BackendHealth { int current = 100; };
    struct BackendNoDefault
    {
        explicit BackendNoDefault(int v) : value(v) {}
        int value;
    };

    class CountingSystem : public ECS::ISystem
    {
    public:
        explicit CountingSystem(int &updates) : _updates(updates) {}
        void update(ECS::Registry &, double) override { _updates++; }
        const std::string name() const override { return "CountingSystem"; }

    private:
        int &_updates;
    };
}

E_REFLECT(BackendHealth) E_FIELD(current, int) E_END
E_REFLECT(BackendNoDefault) E_FIELD(value, int) E_END

void run_ecs_backend_tests()
{
    "the Hierarchy, the Inspector and Play all work on the given world"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        int updates = 0;
        systems.addSystem<CountingSystem>(world, updates);
        EcsBackend backend(world, systems);

        const HierarchyModel::ObjectId id = backend.hierarchy().create(HierarchyModel::InvalidId, "created");
        expect(world.entityCount() == 1_ul);
        expect(backend.properties().addComponent(id, "BackendHealth"));
        expect(backend.properties().set(id, "BackendHealth", "current", 42));
        int current = 0;
        for (auto [entity, health] : world.view<BackendHealth>()) {
            (void)entity;
            current = health.current;
        }
        expect(current == 42_i);

        backend.simulation().begin();
        backend.simulation().update(0.016);
        backend.simulation().end();
        expect(updates == 1_i);
    };

    "the description counts entities, component types and systems"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        int updates = 0;
        world.spawnEntity("one");
        systems.addSystem<CountingSystem>(world, updates);
        EcsBackend backend(world, systems);
        expect(backend.description().starts_with("Editing an ECS world: 1 entity, "));
        expect(backend.description().ends_with(", 1 system"));
    };

    "the components command lists the reflected types"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        EcsBackend backend(world, systems);
        CommandRegistry commands;
        backend.registerCommands(commands);
        Logger::instance().clear();
        expect(commands.execute("components"));
        expect(logged(LogLevel::Info, "BackendHealth"));
        expect(logged(LogLevel::Info, "BackendNoDefault (no default constructor"));
    };

    "the Viewport draws nothing until the game gives a renderer"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        EcsBackend backend(world, systems);
        const Selection selection;
        expect(!backend.renderer(selection));
    };
}
