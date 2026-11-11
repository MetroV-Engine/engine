// EcsScene: what the Hierarchy and the Inspector see of an ECS world.

#include <boost/ut.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "ecs/EcsScene/EcsScene.hpp"

using namespace boost::ut;

// The TypeRegistry is process-wide: names are prefixed with "Scene" so they
// never collide with another test file's E_REFLECT.
namespace {
    struct SceneVector2 { float x = 0, y = 0; };
    struct SceneRgba { float r = 1, g = 1, b = 1, a = 1; };
    struct SceneRange { float min = 0, max = 1; };
    enum class SceneMode : std::int8_t { A, B, C };
    struct SceneMover
    {
        SceneVector2 position;
        double speed = 1.5;
        std::uint8_t small = 10;
        std::int64_t big = 5;
        std::uint32_t unsignedValue = 3;
        SceneMode mode = SceneMode::A;
        std::string label = "hi";
        SceneRgba tint;
        SceneRange range;
        std::vector<int> list{1, 2, 3};
        std::optional<int> maybe;
    };
    struct SceneTag {};
    struct SceneNoDefault
    {
        explicit SceneNoDefault(int v) : value(v) {}
        int value;
    };
    struct SceneUnreflected { int value = 0; };
}

E_REFLECT(SceneVector2) E_FIELD(x, float) E_FIELD(y, float) E_END
E_REFLECT(SceneRgba) E_FIELD(r, float) E_FIELD(g, float) E_FIELD(b, float) E_FIELD(a, float) E_END
E_REFLECT(SceneRange) E_FIELD(min, float) E_FIELD(max, float) E_END
E_REFLECT(SceneMover)
    E_FIELD(position, SceneVector2)
    E_FIELD(speed, double)
    E_FIELD(small, std::uint8_t)
    E_FIELD(big, std::int64_t)
    E_FIELD(unsignedValue, std::uint32_t)
    E_FIELD(mode, SceneMode)
    E_FIELD(label, std::string)
    E_FIELD(tint, SceneRgba)
    E_FIELD(range, SceneRange)
    E_FIELD(list, std::vector<int>)
    E_FIELD(maybe, std::optional<int>)
E_END
E_REFLECT(SceneTag) E_END
E_REFLECT(SceneNoDefault) E_FIELD(value, int) E_END

namespace {
    using Id = HierarchyModel::ObjectId;
    constexpr Id None = HierarchyModel::InvalidId;

    template <typename T>
    bool holds(const PropertyValue &value, const T &expected)
    {
        const T *typed = std::get_if<T>(&value);
        return typed && *typed == expected;
    }

    bool empty(const PropertyValue &value) { return std::holds_alternative<std::monostate>(value); }

    const ComponentInfo *component(const std::vector<ComponentInfo> &components, const std::string &name)
    {
        const auto found = std::ranges::find(components, name, &ComponentInfo::name);
        return found == components.end() ? nullptr : &*found;
    }

    const PropertyInfo *property(const ComponentInfo &info, const std::string &name)
    {
        const auto found = std::ranges::find(info.properties, name, &PropertyInfo::name);
        return found == info.properties.end() ? nullptr : &*found;
    }

    bool contains(const std::vector<std::string> &names, const std::string &name)
    {
        return std::ranges::find(names, name) != names.end();
    }
}

void run_ecs_scene_tests()
{
    // ---- Hierarchy

    "an empty world gives an empty, flat hierarchy"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        expect(scene.roots().empty());
        expect(!scene.supportsParenting());
    };

    "create spawns a named entity at the root, children are refused"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const Id a = scene.create(None, "A");
        expect((a != None) >> fatal);
        expect(scene.exists(a));
        expect(scene.name(a) == "A");
        expect(world.entityCount() == 1_ul);
        expect(world.getComponent<ECS::Name>(*scene.entity(a)).value == "A");
        expect(scene.roots() == std::vector<Id>{a});
        expect(scene.children(a).empty());
        expect(scene.parent(a) == None);

        expect(scene.create(a, "child") == None);
        expect(scene.setParent(a, None));
        expect(!scene.setParent(a, a));
        expect(world.entityCount() == 1_ul);
    };

    "entities spawned by the game appear, named Entity <index> until renamed"_test = [] {
        ECS::Registry world;
        const ECS::Entity raw = world.spawnEntity();
        EcsScene scene(world);
        const Id id = scene.id(raw);
        expect(scene.roots() == std::vector<Id>{id});
        expect(scene.name(id) == "Entity " + std::to_string(raw.index()));
        expect(scene.idLabel(id) == "Entity " + std::to_string(raw.index()) + ", generation 0");

        expect(scene.rename(id, "Renamed"));
        expect(scene.name(id) == "Renamed");
        expect(world.getComponent<ECS::Name>(raw).value == "Renamed");
        expect(!scene.rename(id, ""));
    };

    "destroy kills the entity, and its id never designates the entity reusing its index"_test = [] {
        ECS::Registry world;
        EcsScene scene(world);
        const Id a = scene.create(None, "A");
        expect(scene.entity(a).has_value() >> fatal);
        const auto index = scene.entity(a)->index();
        expect(scene.destroy(a));
        expect(world.entityCount() == 0_ul);
        expect(!scene.exists(a));
        expect(!scene.destroy(a));

        const Id b = scene.create(None, "B");
        expect(scene.entity(b).has_value() >> fatal);
        expect(scene.entity(b)->index() == index) << "the ECS reuses the index of A";
        expect(b != a);
        expect(scene.exists(b));
        expect(!scene.exists(a));
        expect(!scene.exists(None));
        expect(!scene.exists(0xFFFFFFFF00000000ull));
    };

    "an entity killed by the game disappears from the editor"_test = [] {
        ECS::Registry world;
        const ECS::Entity raw = world.spawnEntity("doomed");
        world.addComponent(raw, SceneMover{});
        EcsScene scene(world);
        const Id id = scene.id(raw);
        world.killEntity(raw);
        expect(!scene.exists(id));
        expect(scene.roots().empty());
        expect(scene.name(id).empty());
        expect(scene.components(id).empty());
        expect(empty(scene.get(id, "SceneMover", "speed")));
        expect(!scene.set(id, "SceneMover", "speed", 1.0f));
    };

    // ---- Inspector

    "the reflected components of an entity are listed, without ECS::Name"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        world.addComponent(e, SceneTag{});
        world.addComponent(e, SceneUnreflected{});
        EcsScene scene(world);
        std::vector<std::string> names;
        for (const ComponentInfo &info : scene.components(scene.id(e)))
            names.push_back(info.name);
        std::ranges::sort(names);
        expect(names == std::vector<std::string>{"SceneMover", "SceneTag"});
    };

    "each field gets the widget of its type"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        EcsScene scene(world);
        const std::vector<ComponentInfo> components = scene.components(scene.id(e));
        const ComponentInfo *mover = component(components, "SceneMover");
        expect((mover != nullptr) >> fatal);
        expect(mover->properties.size() == 11_ul);
        for (const char *name : {"position", "speed", "small", "big", "mode", "label", "tint", "range", "list", "maybe"})
            expect((property(*mover, name) != nullptr) >> fatal) << name;

        expect(property(*mover, "position")->type == PropertyType::Vec2);
        expect(property(*mover, "speed")->type == PropertyType::Float);
        expect(property(*mover, "small")->type == PropertyType::Int);
        // Integers narrower than int get their range as bounds, the others none.
        expect(property(*mover, "small")->min == 0.0f && property(*mover, "small")->max == 255.0f);
        expect(property(*mover, "big")->min == property(*mover, "big")->max);
        expect(property(*mover, "mode")->type == PropertyType::Int);
        expect(property(*mover, "label")->type == PropertyType::String && !property(*mover, "label")->readOnly);
        expect(property(*mover, "tint")->type == PropertyType::Color);
        expect(property(*mover, "range")->type == PropertyType::Group);
        expect(property(*mover, "range")->fields.size() == 2_ul);
        // Containers are shown as a read-only summary.
        expect(property(*mover, "list")->type == PropertyType::String && property(*mover, "list")->readOnly);
        expect(property(*mover, "maybe")->readOnly);
    };

    "get reads the values of the world"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        EcsScene scene(world);
        const Id id = scene.id(e);
        expect(holds(scene.get(id, "SceneMover", "position"), Vec2{0, 0}));
        expect(holds(scene.get(id, "SceneMover", "speed"), 1.5f));
        expect(holds(scene.get(id, "SceneMover", "small"), 10));
        expect(holds(scene.get(id, "SceneMover", "mode"), 0));
        expect(holds(scene.get(id, "SceneMover", "label"), std::string("hi")));
        expect(holds(scene.get(id, "SceneMover", "tint"), Color{1, 1, 1, 1}));
        expect(holds(scene.get(id, "SceneMover", "range.max"), 1.0f));
        expect(holds(scene.get(id, "SceneMover", "list"), std::string("3 elements")));
        expect(holds(scene.get(id, "SceneMover", "maybe"), std::string("(empty)")));

        expect(empty(scene.get(id, "SceneMover", "range"))); // a group has no value itself
        expect(empty(scene.get(id, "SceneMover", "nope")));
        expect(empty(scene.get(id, "SceneMover", "range..max")));
        expect(empty(scene.get(id, "SceneMover", "")));
        expect(empty(scene.get(id, "Unknown", "x")));
    };

    "set writes into the world, integers clamped to their type"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        EcsScene scene(world);
        const Id id = scene.id(e);
        const SceneMover &mover = world.getComponent<SceneMover>(e);

        expect(scene.set(id, "SceneMover", "position", Vec2{3, 4}));
        expect(mover.position.x == 3.0f && mover.position.y == 4.0f);
        expect(scene.set(id, "SceneMover", "speed", 2.25f));
        expect(mover.speed == 2.25);
        expect(scene.set(id, "SceneMover", "big", 42));
        expect(mover.big == 42);
        expect(scene.set(id, "SceneMover", "small", 300));
        expect(mover.small == 255);
        expect(scene.set(id, "SceneMover", "small", -5));
        expect(mover.small == 0);
        expect(scene.set(id, "SceneMover", "unsignedValue", -7));
        expect(mover.unsignedValue == 0u);
        expect(scene.set(id, "SceneMover", "mode", 2));
        expect(mover.mode == SceneMode::C);
        expect(scene.set(id, "SceneMover", "mode", 1000));
        expect(static_cast<int>(mover.mode) == 127); // clamped to the underlying int8_t
        expect(scene.set(id, "SceneMover", "label", std::string("yo")));
        expect(mover.label == "yo");
        expect(scene.set(id, "SceneMover", "tint", Color{0.1f, 0.2f, 0.3f, 0.4f}));
        expect(mover.tint.b == 0.3f);
        expect(scene.set(id, "SceneMover", "range.max", 5.0f));
        expect(mover.range.max == 5.0f);
    };

    "set refuses read-only fields, wrong types and unknown paths"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        EcsScene scene(world);
        const Id id = scene.id(e);
        expect(!scene.set(id, "SceneMover", "speed", true));
        expect(!scene.set(id, "SceneMover", "list", std::string("x")));
        expect(!scene.set(id, "SceneMover", "range", 1.0f));
        expect(!scene.set(id, "SceneMover", "nope", 1.0f));
        expect(!scene.set(id, "Unknown", "speed", 1.0f));
        expect(world.getComponent<SceneMover>(e).speed == 1.5);
    };

    "Add Component lists the reflected types that can be added, sorted"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        world.addComponent(e, SceneMover{});
        EcsScene scene(world);
        const std::vector<std::string> addable = scene.addableComponents(scene.id(e));
        expect(std::ranges::is_sorted(addable));
        expect(contains(addable, "SceneTag") && contains(addable, "SceneRange"));
        expect(!contains(addable, "SceneMover")) << "already present";
        expect(!contains(addable, "ECS::Name")) << "edited as the entity's name";
        expect(!contains(addable, "SceneNoDefault")) << "no default constructor";
    };

    "add and remove components"_test = [] {
        ECS::Registry world;
        const ECS::Entity e = world.spawnEntity("e");
        EcsScene scene(world);
        const Id id = scene.id(e);
        expect(scene.addComponent(id, "SceneTag"));
        expect(world.hasComponents<SceneTag>(e));
        expect(!scene.addComponent(id, "SceneTag"));
        expect(!scene.addComponent(id, "SceneNoDefault"));
        expect(!scene.addComponent(id, "ECS::Name"));
        expect(!scene.addComponent(id, "Unknown"));

        expect(scene.removeComponent(id, "SceneTag"));
        expect(!world.hasComponents<SceneTag>(e));
        expect(!scene.removeComponent(id, "SceneTag"));
        expect(!scene.removeComponent(id, "ECS::Name"));
        expect(world.hasComponents<ECS::Name>(e));
    };
}
