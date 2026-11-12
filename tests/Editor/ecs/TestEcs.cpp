#include <boost/ut.hpp>

#include "Support/Headless.hpp"
#include "core/CommandRegistry/CommandRegistry.hpp"
#include "ecs/EcsBackend/EcsBackend.hpp"
#include "ecs/EcsScene/EcsScene.hpp"
#include "ecs/EcsSimulation/EcsSimulation.hpp"
#include "TestComponents.hpp"

#include "ECS/ECS.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace boost::ut;
using namespace EditorTest;
using ObjectId = HierarchyModel::ObjectId;

namespace {

const PropertyInfo *property(const std::vector<PropertyInfo> &properties, const std::string &name)
{
    for (const PropertyInfo &info : properties)
        if (info.name == name)
            return &info;
    return nullptr;
}

const ComponentInfo *component(const std::vector<ComponentInfo> &components, const std::string &name)
{
    for (const ComponentInfo &info : components)
        if (info.name == name)
            return &info;
    return nullptr;
}

} // namespace

void run_ecs_scene_tests()
{
    "ids: every live entity, named or not; a killed entity's id never comes back"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity named = world.spawnEntity("Hero");
        const ECS::Entity anonymous = world.spawnEntity();
        const std::vector<ObjectId> roots = scene.roots();
        expect(roots.size() == 2_u);
        const ObjectId heroId = scene.id(named);
        expect(scene.name(heroId) == "Hero");
        expect(scene.name(scene.id(anonymous)).starts_with("Entity"));
        expect(scene.entity(heroId) == std::optional<ECS::Entity>(named));
        expect(scene.idLabel(heroId).starts_with("Entity "));
        expect(scene.children(heroId).empty());
        expect(scene.parent(heroId) == HierarchyModel::InvalidId);
        expect(!scene.supportsParenting());
        expect(&scene.world() == &world);

        world.killEntity(named);
        const ECS::Entity reused = world.spawnEntity("Reused");
        expect(!scene.exists(heroId));
        expect(scene.name(heroId).empty());
        expect(scene.idLabel(heroId) == "No entity");
        expect(scene.id(reused) != heroId);
        expect(scene.id(named) == HierarchyModel::InvalidId);
        expect(!scene.entity(HierarchyModel::InvalidId).has_value());
        expect(!scene.entity(0xFFFFull).has_value()); // Index never given out
    };

    "hierarchy edits: create at the root only, rename, delete, move to the root"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ObjectId id = scene.create(HierarchyModel::InvalidId, "New Object");
        expect(id != HierarchyModel::InvalidId);
        expect(scene.create(id, "Child") == HierarchyModel::InvalidId);
        expect(scene.rename(id, "Renamed"));
        expect(scene.name(id) == "Renamed");
        expect(!scene.rename(id, ""));
        const ObjectId bare = scene.id(world.spawnEntity());
        expect(scene.rename(bare, "Named now")); // Adds an ECS::Name
        expect(scene.name(bare) == "Named now");
        expect(scene.setParent(id, HierarchyModel::InvalidId));
        expect(!scene.setParent(id, bare));
        expect(scene.destroy(id));
        expect(!scene.destroy(id));
        expect(!scene.rename(id, "Gone"));
    };

    "components: reflected ones with their fields, ECS::Name left out"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity entity = world.spawnEntity("Thing");
        world.addComponent(entity, EdStats{});
        world.addComponent(entity, EdShape{});
        world.addComponent(entity, EdContainers{});
        world.addComponent(entity, EdUnreflected{});
        const ObjectId id = scene.id(entity);
        const std::vector<ComponentInfo> components = scene.components(id);
        expect(components.size() == 3_u);
        expect(component(components, "ECS::Name") == nullptr);
        expect(scene.components(HierarchyModel::InvalidId).empty());

        const ComponentInfo &stats = *component(components, "EdStats");
        expect(property(stats.properties, "alive")->type == PropertyType::Bool);
        const PropertyInfo &level = *property(stats.properties, "level");
        expect(level.type == PropertyType::Int);
        expect(level.min == -128.0_f and level.max == 127.0_f);
        expect(property(stats.properties, "gold")->min == property(stats.properties, "gold")->max); // Unbounded
        expect(property(stats.properties, "speed")->type == PropertyType::Float);
        expect(property(stats.properties, "precise")->type == PropertyType::Float);
        expect(property(stats.properties, "title")->type == PropertyType::String);
        const PropertyInfo &kind = *property(stats.properties, "kind");
        expect(kind.type == PropertyType::Int and kind.tooltip.starts_with("Enum"));

        const ComponentInfo &shape = *component(components, "EdShape");
        expect(property(shape.properties, "position")->type == PropertyType::Vec2);
        expect(property(shape.properties, "tint")->type == PropertyType::Color);
        const PropertyInfo &movement = *property(shape.properties, "movement");
        expect(movement.type == PropertyType::Group);
        expect(movement.fields.size() == 2_u);

        const ComponentInfo &containers = *component(components, "EdContainers");
        for (const PropertyInfo &info : containers.properties)
            expect(info.readOnly and info.type == PropertyType::String) << info.name;

        // Described once per type.
        expect(scene.components(id).size() == 3_u);
    };

    "get and set: each kind of value, integers saturated to their type"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity entity = world.spawnEntity("Thing");
        world.addComponent(entity, EdStats{});
        world.addComponent(entity, EdShape{});
        const ObjectId id = scene.id(entity);
        const auto stats = [&]() -> EdStats & { return world.getComponent<EdStats>(entity); };

        expect(scene.set(id, "EdStats", "alive", false));
        expect(!stats().alive);
        expect(scene.get(id, "EdStats", "alive") == PropertyValue(false));
        expect(scene.set(id, "EdStats", "level", 1000));
        expect(stats().level == 127);
        expect(scene.set(id, "EdStats", "a", -5));
        expect(stats().a == 0);
        expect(scene.set(id, "EdStats", "xp", 42));
        expect(stats().xp == 42);
        stats().xp = std::numeric_limits<std::int64_t>::max();
        expect(scene.get(id, "EdStats", "xp") == PropertyValue(std::numeric_limits<int>::max()));
        stats().d = std::numeric_limits<std::uint64_t>::max();
        expect(scene.get(id, "EdStats", "d") == PropertyValue(std::numeric_limits<int>::max()));
        expect(scene.set(id, "EdStats", "speed", 2.5f));
        expect(stats().speed == 2.5f);
        expect(scene.set(id, "EdStats", "mass", 70.0f));
        expect(stats().mass == 70.0);
        stats().mass = 1e300;
        expect(scene.get(id, "EdStats", "mass") == PropertyValue(std::numeric_limits<float>::max()));
        stats().mass = std::numeric_limits<double>::infinity();
        expect(std::isinf(std::get<float>(scene.get(id, "EdStats", "mass"))));
        expect(scene.set(id, "EdStats", "precise", 1.5f));
        expect(scene.set(id, "EdStats", "title", std::string("Knight")));
        expect(stats().title == "Knight");
        expect(scene.set(id, "EdStats", "kind", 2));
        expect(stats().kind == EdKind::Boss);
        expect(scene.get(id, "EdStats", "kind") == PropertyValue(2));
        expect(scene.set(id, "EdStats", "kind", 1000)); // Saturated to the underlying uint8
        expect(static_cast<int>(stats().kind) == 255);

        expect(scene.set(id, "EdShape", "position", Vec2{1, 2}));
        expect(scene.get(id, "EdShape", "position") == PropertyValue(Vec2{1, 2}));
        expect(scene.set(id, "EdShape", "tint", Color{0.1f, 0.2f, 0.3f, 0.4f}));
        expect(scene.get(id, "EdShape", "tint") == PropertyValue(Color{0.1f, 0.2f, 0.3f, 0.4f}));
        expect(scene.set(id, "EdShape", "movement.speed", 3.0f));
        expect(world.getComponent<EdShape>(entity).movement.speed == 3.0f);
        expect(scene.set(id, "EdShape", "movement.direction", Vec2{0, 1}));
        expect(std::holds_alternative<std::monostate>(scene.get(id, "EdShape", "movement")));

        // Wrong types, unknown paths, groups.
        for (const auto &[path, value] : std::vector<std::pair<std::string, PropertyValue>>{{"alive", 1},
                 {"level", true}, {"speed", 1}, {"title", 1.0f}, {"kind", 1.0f}}) {
            expect(!scene.set(id, "EdStats", path, value)) << path;
        }
        expect(!scene.set(id, "EdShape", "position", 1.0f));
        expect(!scene.set(id, "EdShape", "tint", Vec2{}));
        expect(!scene.set(id, "EdShape", "movement", 1.0f));
        expect(!scene.set(id, "EdStats", "nope", 1));
        expect(!scene.set(id, "EdStats", "", 1));
        expect(!scene.set(id, "EdStats", "alive.inner", true));
        expect(!scene.set(id, "Unknown", "alive", true));
        expect(!scene.set(HierarchyModel::InvalidId, "EdStats", "alive", true));
        expect(!scene.set(id, "EdContainers", "path", std::string("x"))); // Not on the entity
        expect(std::holds_alternative<std::monostate>(scene.get(id, "EdStats", "nope")));
    };

    "read-only summaries: containers, optionals, opaque values, characters"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity entity = world.spawnEntity("Thing");
        world.addComponent(entity, EdContainers{});
        const ObjectId id = scene.id(entity);
        expect(scene.get(id, "EdContainers", "path") == PropertyValue(std::string("0 elements")));
        world.getComponent<EdContainers>(entity).path = {1};
        expect(scene.get(id, "EdContainers", "path") == PropertyValue(std::string("1 element")));
        world.getComponent<EdContainers>(entity).path = {1, 2};
        expect(scene.get(id, "EdContainers", "path") == PropertyValue(std::string("2 elements")));
        expect(scene.get(id, "EdContainers", "tags") == PropertyValue(std::string("0 elements")));
        expect(scene.get(id, "EdContainers", "target") == PropertyValue(std::string("(empty)")));
        world.getComponent<EdContainers>(entity).target = 3;
        expect(scene.get(id, "EdContainers", "target") == PropertyValue(std::string("(set)")));
        expect(scene.get(id, "EdContainers", "pair") == PropertyValue(std::string("(not inspectable)")));
        expect(scene.get(id, "EdContainers", "letter") == PropertyValue(std::string("z")));
        expect(!scene.set(id, "EdContainers", "letter", std::string("y")));
    };

    "add and remove components: reflected, default-constructible, not ECS::Name"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ECS::Entity entity = world.spawnEntity("Thing");
        const ObjectId id = scene.id(entity);
        const std::vector<std::string> addable = scene.addableComponents(id);
        expect(std::ranges::is_sorted(addable));
        expect(std::ranges::find(addable, "EdStats") != addable.end());
        expect(std::ranges::find(addable, "EdNoDefault") == addable.end());
        expect(std::ranges::find(addable, "ECS::Name") == addable.end());
        expect(scene.addableComponents(HierarchyModel::InvalidId).empty());

        expect(scene.addComponent(id, "EdStats"));
        expect(world.hasComponents<EdStats>(entity));
        expect(!scene.addComponent(id, "EdStats"));
        expect(!scene.addComponent(id, "ECS::Name"));
        expect(!scene.addComponent(id, "Unknown"));
        expect(!scene.addComponent(HierarchyModel::InvalidId, "EdStats"));
        expect(scene.removeComponent(id, "EdStats"));
        expect(!scene.removeComponent(id, "EdStats"));
        expect(!scene.removeComponent(id, "ECS::Name"));
        expect(!scene.removeComponent(id, "Unknown"));
        expect(!scene.removeComponent(HierarchyModel::InvalidId, "EdStats"));
    };

    "a component constructor that throws is logged, not fatal"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const ObjectId id = scene.id(world.spawnEntity("Thing"));
        LogWatch log;
        EdFragile::failConstruct = true;
        expect(!scene.addComponent(id, "EdFragile"));
        EdFragile::failConstruct = false;
        expect(log.contains(LogLevel::Error, "Cannot add EdFragile: construction failed"));
    };
}

namespace {

// Moves and changes the world each update, like a game.
class ChaosSystem : public ECS::ISystem
{
public:
    void update(ECS::Registry &world, double) override
    {
        for (auto [entity, stats] : world.view<EdStats>())
            stats.gold += 10;
        world.spawnEntity("Spawned during play");
    }
    const std::string name() const override { return "ChaosSystem"; }
};

} // namespace

void run_ecs_simulation_tests()
{
    "Stop puts the world back as it was on Play"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        systems.addSystem<ChaosSystem>(world);
        EcsSimulation simulation(world, systems);

        const ECS::Entity hero = world.spawnEntity("Hero");
        world.addComponent(hero, EdStats{});
        world.addComponent(hero, EdShape{});
        const ECS::Entity victim = world.spawnEntity("Victim");
        world.addComponent(victim, EdStats{});
        const std::size_t before = world.entityCount();

        simulation.begin();
        simulation.update(1.0 / 60.0);
        simulation.update(1.0 / 60.0);
        expect(world.getComponent<EdStats>(hero).gold == 20);
        world.removeComponent<EdShape>(hero);  // Removed during play
        world.addComponent(hero, EdContainers{}); // Added during play
        world.killEntity(victim);                  // Killed during play

        simulation.end();
        expect(world.entityCount() == before);
        expect(world.isAlive(hero));
        expect(world.getComponent<EdStats>(hero).gold == 0);
        expect(world.hasComponents<EdShape>(hero));
        expect(!world.hasComponents<EdContainers>(hero));
        // The victim is back, with a new handle and its components.
        bool victimBack = false;
        for (auto [entity, name] : world.view<ECS::Name>())
            if (name.value == "Victim")
                victimBack = world.hasComponents<EdStats>(entity);
        expect(victimBack);
    };

    "components that cannot be copied or recreated are reported"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        EcsSimulation simulation(world, systems);
        const ECS::Entity keeper = world.spawnEntity("Keeper");
        world.addComponent(keeper, EdMoveOnly{});
        world.addComponent(keeper, EdNoDefault{5});
        const ECS::Entity lost = world.spawnEntity("Lost");
        world.addComponent(lost, EdMoveOnly{});
        LogWatch log;

        simulation.begin();
        expect(log.contains(LogLevel::Warning, "Not copyable, so not restored on Stop: EdMoveOnly"));
        world.getComponent<EdMoveOnly>(keeper).value = 9;
        world.removeComponent<EdNoDefault>(keeper);
        world.killEntity(lost);
        simulation.end();
        expect(world.getComponent<EdMoveOnly>(keeper).value == 9); // Kept as it was
        expect(log.contains(LogLevel::Warning, "EdNoDefault (no default constructor)"));
        expect(log.contains(LogLevel::Warning, "Not restored on Stop: EdNoDefault (no default constructor), EdMoveOnly"));
    };

    "a copy that throws: Play is refused, or the component gets a default value back"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        EcsSimulation simulation(world, systems);
        const ECS::Entity entity = world.spawnEntity("Fragile");
        world.addComponent(entity, EdFragile{});

        EdFragile::failCopy = true;
        expect(throws([&] { simulation.begin(); }));
        EdFragile::failCopy = false;

        simulation.begin();
        world.getComponent<EdFragile>(entity).value = 7;
        LogWatch log;
        EdFragile::failCopy = true;
        simulation.end();
        EdFragile::failCopy = false;
        expect(log.contains(LogLevel::Warning, "EdFragile (copy failed)"));
        expect(world.getComponent<EdFragile>(entity).value == 0);
    };

    "EcsBackend: its description, its \"components\" command, reflection errors"_test = [] {
        ECS::Registry world;
        ECS::SystemManager systems;
        world.spawnEntity("One");
        LogWatch log;
        EcsBackend backend(world, systems);
        expect(backend.description().starts_with("Editing an ECS world: 1 entity, "));
        expect(backend.description().ends_with("0 systems"));
        systems.addSystem<ChaosSystem>(world);
        world.spawnEntity("Two");
        expect(backend.description().find("2 entities") != std::string::npos);
        expect(backend.description().ends_with("1 system"));
        expect(backend.hierarchy().roots().size() == 2_u);
        expect(backend.properties().components(backend.hierarchy().roots().front()).empty());
        backend.simulation().begin();
        backend.simulation().end();

        CommandRegistry commands;
        backend.registerCommands(commands);
        expect(commands.execute("components"));
        expect(log.contains("component types"));
        expect(log.contains("EdNoDefault (no default constructor: cannot be added)"));
    };
}
