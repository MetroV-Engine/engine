#include <boost/ut.hpp>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ECS/Ref.hpp"

using namespace boost::ut;

// The registry is a process-wide singleton: names are prefixed with "Rc" so
// they never collide with another test file's E_REFLECT.
namespace {
    enum class RcState : std::uint8_t { Idle, Running, Dead };

    struct RcItem { std::string id; int count; };

    struct RcWeapon { int damage; };

    struct RcInventory {
        std::vector<RcItem> items;
        std::array<float, 3> color;
        std::map<std::string, int> counters;
        std::unordered_map<int, std::string> labels;
        std::optional<RcWeapon> weapon;
        RcState state;
        std::map<RcState, int> perState;
    };

    struct RcNode { int value; std::vector<RcNode> children; };
    struct RcTree { int value; std::vector<RcTree> children; };

    struct RcNoDefault {
        explicit RcNoDefault(int v) : value(v) {}
        int value;
    };

    // Map keys that path() cannot build from text.
    struct RcKeyNoDefault {
        explicit RcKeyNoDefault(int v) : value(v) {}
        int value;
        bool operator<(const RcKeyNoDefault& other) const { return value < other.value; }
    };

    struct RcThrowingKey {
        RcThrowingKey() { throw std::runtime_error("RcThrowingKey"); }
        explicit RcThrowingKey(int v) : value(v) {}
        int value = 0;
        bool operator<(const RcThrowingKey& other) const { return value < other.value; }
    };

    // Self-references through a map and through an optional.
    struct RcGraph { std::map<std::string, std::vector<RcGraph>> links; };
    struct RcChain { std::optional<std::vector<RcChain>> next; };

    // A user container made inspectable through its own ContainerTraits.
    struct RcStack { std::vector<int> data; };
}

E_REFLECT(RcItem)
    E_FIELD(id, std::string)
    E_FIELD(count, int)
E_END

E_REFLECT(RcWeapon)
    E_FIELD(damage, int)
E_END

E_REFLECT(RcInventory)
    E_FIELD(items, std::vector<RcItem>)
    E_FIELD(color, std::array<float, 3>)
    E_FIELD(counters, std::map<std::string, int>)
    E_FIELD(labels, std::unordered_map<int, std::string>)
    E_FIELD(weapon, std::optional<RcWeapon>)
    E_FIELD(state, RcState)
    E_FIELD(perState, std::map<RcState, int>)
E_END

E_REFLECT(RcNode)
    E_FIELD(value, int)
    E_FIELD(children, std::vector<RcNode>)
E_END

E_REFLECT(RcTree)
    E_FIELD(value, int)
    E_FIELD(children, std::vector<RcTree>)
E_END

E_REFLECT(RcNoDefault)
    E_FIELD(value, int)
E_END

E_REFLECT(RcGraph)
    E_FIELD(links, std::map<std::string, std::vector<RcGraph>>)
E_END

E_REFLECT(RcChain)
    E_FIELD(next, std::optional<std::vector<RcChain>>)
E_END

template<>
struct ECS::reflect::ContainerTraits<RcStack> {
    static constexpr TypeKind kind = TypeKind::Sequence;
    using Element = int;

    static std::string name(std::string_view element) {
        return "stack<" + std::string(element) + ">";
    }
    static std::size_t size(const RcStack& s) noexcept { return s.data.size(); }
    static int* at(RcStack& s, std::size_t index) noexcept { return &s.data[index]; }
};

using ECS::reflect::Ref;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;
using ECS::reflect::TypeRegistry;

namespace {
    TypeRegistry& registry() { return TypeRegistry::instance(); }

    RcInventory makeInventory() {
        return RcInventory{
            {{"sword", 1}, {"potion", 3}},
            {0.1f, 0.2f, 0.3f},
            {{"gold", 50}, {"gems", 2}},
            {{7, "seven"}},
            std::nullopt,
            RcState::Running,
            {{RcState::Running, 10}},
        };
    }

void test_container_kinds_and_names() {
    "containers get their kind and a portable name"_test = [] {
        const TypeInfo& inventory = registry().get<RcInventory>();
        auto fieldType = [&](std::string_view name) { return inventory.findField(name)->type; };

        expect(fieldType("items")->kind == TypeKind::Sequence);
        expect(fieldType("items")->name == "vector<RcItem>");
        expect(fieldType("color")->kind == TypeKind::Sequence);
        expect(fieldType("color")->name == "array<float,3>");
        expect(fieldType("counters")->kind == TypeKind::Map);
        expect(fieldType("counters")->name == "map<string,int32>");
        expect(fieldType("labels")->name == "unordered_map<int32,string>");
        expect(fieldType("weapon")->kind == TypeKind::Optional);
        expect(fieldType("weapon")->name == "optional<RcWeapon>");
        expect(fieldType("state")->kind == TypeKind::Enum);
        expect(registry().find("vector<RcItem>") == fieldType("items"));
    };
}

void test_container_ops_describe_element_types() {
    "container ops point to their element types"_test = [] {
        const TypeInfo& inventory = registry().get<RcInventory>();

        expect(inventory.findField("items")->type->sequenceOps->element == &registry().get<RcItem>());
        expect(inventory.findField("counters")->type->mapOps->key == registry().find<std::string>());
        expect(inventory.findField("counters")->type->mapOps->value == registry().find<int>());
        expect(inventory.findField("weapon")->type->optionalOps->value == &registry().get<RcWeapon>());
        expect(inventory.findField("state")->type->enumOps->underlying == registry().find<std::uint8_t>());
    };
}

void test_sequence_size_and_at() {
    "a Sequence exposes its size and elements by index"_test = [] {
        RcInventory inventory = makeInventory();
        Ref items = Ref::make(inventory).field("items");

        expect(items.size() == 2_ul);
        expect(items.at(1).field("id").as<std::string>() == std::string("potion"));
        expect(!items.at(2).valid());
        expect(!Ref::make(inventory).field("state").at(0).valid());   // not a Sequence
    };
}

void test_vector_resize_push_erase() {
    "a vector can be resized, pushed to and erased from"_test = [] {
        RcInventory inventory = makeInventory();
        Ref items = Ref::make(inventory).field("items");

        expect(items.resizable());
        Ref pushed = items.push();
        expect((pushed.valid()) >> fatal);
        pushed.field("id").as<std::string>() = "shield";

        expect(inventory.items.size() == 3_ul);
        expect(inventory.items[2].id == std::string("shield"));

        expect(items.erase(0));
        expect(inventory.items.front().id == std::string("potion"));
        expect(!items.erase(5));

        expect(items.resize(0));
        expect(inventory.items.empty());
    };
}

void test_array_is_fixed_size() {
    "a std::array is a fixed-size Sequence"_test = [] {
        RcInventory inventory = makeInventory();
        Ref color = Ref::make(inventory).field("color");

        expect(color.size() == 3_ul);
        expect(!color.resizable());
        expect(!color.resize(4));
        expect(!color.push().valid());
        expect(!color.erase(0));

        color.at(2).as<float>() = 1.0f;
        expect(inventory.color[2] == 1.0_f);
    };
}

void test_vector_without_default_element() {
    "a vector of non default-constructible elements cannot grow but can erase"_test = [] {
        std::vector<RcNoDefault> values{RcNoDefault{1}, RcNoDefault{2}};
        Ref ref = Ref::make(values);

        expect(!ref.resizable());
        expect(!ref.push().valid());
        expect(ref.erase(0));
        expect(values.size() == 1_ul);
        expect(ref.at(0).field("value").as<int>() == 2_i);
    };
}

void test_map_find_insert_erase() {
    "a Map finds, inserts and erases by key"_test = [] {
        RcInventory inventory = makeInventory();
        Ref counters = Ref::make(inventory).field("counters");
        std::string gold = "gold";
        std::string wood = "wood";

        expect(counters.size() == 2_ul);
        expect(counters.find(Ref::make(gold)).as<int>() == 50_i);
        expect(!counters.find(Ref::make(wood)).valid());

        counters.insert(Ref::make(wood)).as<int>() = 4;
        expect(inventory.counters.at("wood") == 4_i);

        expect(counters.erase(Ref::make(gold)));
        expect(!counters.erase(Ref::make(gold)));
        expect(inventory.counters.count("gold") == 0_ul);
    };
}

void test_map_rejects_wrong_key_type() {
    "a key of the wrong type is rejected"_test = [] {
        RcInventory inventory = makeInventory();
        Ref counters = Ref::make(inventory).field("counters");
        int notAString = 3;

        expect(!counters.find(Ref::make(notAString)).valid());
        expect(!counters.insert(Ref::make(notAString)).valid());
        expect(!counters.erase(Ref::make(notAString)));
        expect(!counters.find(Ref{}).valid());
    };
}

void test_map_for_each_entry() {
    "forEachEntry visits every key/value pair"_test = [] {
        RcInventory inventory = makeInventory();
        Ref counters = Ref::make(inventory).field("counters");
        int total = 0;
        std::set<std::string> keys;

        counters.forEachEntry([&](Ref key, Ref value) {
            keys.insert(key.as<std::string>());
            total += value.as<int>();
            value.as<int>() += 1;
        });

        expect(keys == std::set<std::string>{"gems", "gold"});
        expect(total == 52_i);
        expect(inventory.counters.at("gold") == 51_i);
    };
}

void test_optional() {
    "an Optional can be inspected, emplaced and reset"_test = [] {
        RcInventory inventory = makeInventory();
        Ref weapon = Ref::make(inventory).field("weapon");

        expect(!weapon.hasValue());
        expect(!weapon.value().valid());

        weapon.emplace().field("damage").as<int>() = 12;
        expect(inventory.weapon.has_value());
        expect(inventory.weapon->damage == 12_i);
        expect(weapon.value().field("damage").as<int>() == 12_i);

        expect(weapon.reset());
        expect(!inventory.weapon.has_value());
    };
}

void test_enum_integer_value() {
    "an Enum is read and written through its integer value"_test = [] {
        RcInventory inventory = makeInventory();
        Ref state = Ref::make(inventory).field("state");

        expect(state.enumValue() == std::optional<std::int64_t>{1});
        expect(state.setEnumValue(2));
        expect(inventory.state == RcState::Dead);
        expect(!Ref::make(inventory).field("items").enumValue().has_value());
        expect(state.tryAs<RcState>() == &inventory.state);
    };
}

void test_path_through_containers() {
    "path reaches elements with [index] and [key]"_test = [] {
        RcInventory inventory = makeInventory();
        Ref root = Ref::make(inventory);

        root.path("items[1].count").as<int>() = 9;
        expect(inventory.items[1].count == 9_i);
        expect(root.path("color[2]").tryAs<float>() == &inventory.color[2]);
        expect(root.path("counters[gold]").as<int>() == 50_i);
        expect(root.path("labels[7]").as<std::string>() == std::string("seven"));
        expect(root.path("perState[1]").as<int>() == 10_i);   // enum key, from its integer value
    };
}

void test_path_through_optional() {
    "path goes through an engaged Optional and fails on an empty one"_test = [] {
        RcInventory inventory = makeInventory();
        Ref root = Ref::make(inventory);

        expect(!root.path("weapon.damage").valid());
        expect(root.path("weapon").is(TypeKind::Optional));   // the Optional itself, not unwrapped

        inventory.weapon = RcWeapon{5};
        expect(root.path("weapon.damage").as<int>() == 5_i);
    };
}

void test_path_container_errors() {
    "path is invalid on bad indices, keys or syntax"_test = [] {
        RcInventory inventory = makeInventory();
        Ref root = Ref::make(inventory);

        expect(!root.path("items[9]").valid());
        expect(!root.path("items[x]").valid());
        expect(!root.path("items[-1]").valid());
        expect(!root.path("items[]").valid());
        expect(!root.path("items[0").valid());
        expect(!root.path("items[0]x").valid());
        expect(!root.path("counters[silver]").valid());
        expect(!root.path("labels[seven]").valid());       // int key not parsable
        expect(!root.path("state[0]").valid());            // not a container
        expect(!root.path("[0]").valid());                 // root is a Struct
        expect(!root.path("items.[0]").valid());
    };
}

void test_path_on_root_container() {
    "a path may start with an index when the root is a container"_test = [] {
        std::vector<std::vector<int>> grid{{1, 2}, {3, 4}};
        Ref ref = Ref::make(grid);

        expect(ref.type().name == "vector<vector<int32>>");
        expect(ref.path("[1][0]").as<int>() == 3_i);
        expect(ref.path("[1]").size() == 2_ul);
    };
}

void test_nested_container_in_map() {
    "containers nest inside maps"_test = [] {
        std::map<std::string, std::vector<int>> groups{{"a", {1, 2, 3}}};
        Ref ref = Ref::make(groups);

        expect(ref.type().name == "map<string,vector<int32>>");
        expect(ref.path("[a][2]").as<int>() == 3_i);
    };
}

void test_self_referencing_types() {
    "self-referencing types register in either order"_test = [] {
        (void)registry().get<RcNode>();                       // struct first
        (void)registry().get<std::vector<RcTree>>();          // container first

        expect(registry().find("vector<RcNode>") != nullptr);
        expect(registry().find("RcTree") != nullptr);
        expect(registry().get<RcTree>().findField("children")->type ==
               registry().find<std::vector<RcTree>>());

        RcNode root{1, {RcNode{2, {RcNode{3, {}}}}}};
        expect(Ref::make(root).path("children[0].children[0].value").as<int>() == 3_i);
    };
}

void test_self_reference_through_map_and_optional() {
    "self-references through a map or an optional register container first"_test = [] {
        using Links = std::map<std::string, std::vector<RcGraph>>;
        using Next = std::optional<std::vector<RcChain>>;
        const TypeInfo& links = registry().get<Links>();
        const TypeInfo& next = registry().get<Next>();

        expect(registry().get<RcGraph>().findField("links")->type == &links);
        expect(registry().get<RcChain>().findField("next")->type == &next);

        RcGraph graph{{{"a", {RcGraph{}}}}};
        expect(Ref::make(graph).path("links[a][0].links").is(TypeKind::Map));
    };
}

void test_operations_on_the_wrong_kind() {
    "container, optional and enum operations fail on another kind"_test = [] {
        RcInventory inventory = makeInventory();
        Ref root = Ref::make(inventory);
        bool called = false;

        expect(root.size() == 0_ul);
        root.field("items").forEachEntry([&](Ref, Ref) { called = true; });
        expect(!called);
        expect(!root.emplace().valid());
        expect(!root.reset());
        expect(!root.field("items").setEnumValue(1));
        expect(!root.field("items").find(Ref::make(inventory)).valid());
    };
}

void test_optional_without_default_value() {
    "emplace fails on an optional of a non default-constructible type"_test = [] {
        std::optional<RcNoDefault> maybe;

        expect(!Ref::make(maybe).emplace().valid());
        expect(!maybe.has_value());
    };
}

void test_path_key_without_default_constructor() {
    "path cannot look up a map whose key has no default constructor"_test = [] {
        std::map<RcKeyNoDefault, int> values{{RcKeyNoDefault{1}, 2}};

        expect(!Ref::make(values).path("[1]").valid());
    };
}

void test_path_key_constructor_throws() {
    "path propagates an exception thrown while building a key"_test = [] {
        std::map<RcThrowingKey, int> values;

        expect(throws<std::runtime_error>([&] { (void)Ref::make(values).path("[1]"); }));
    };
}

void test_vector_bool_stays_opaque() {
    "std::vector<bool> is Opaque since its elements have no address"_test = [] {
        expect(registry().get<std::vector<bool>>().kind == TypeKind::Opaque);
    };
}

void test_copy_respects_element_copyability() {
    "copy is null for a container of non-copyable elements"_test = [] {
        expect(registry().get<std::vector<std::unique_ptr<int>>>().copy == nullptr);

        const TypeInfo& ints = registry().get<std::vector<int>>();
        expect((ints.copy != nullptr) >> fatal);
        std::vector<int> source{1, 2, 3};
        alignas(std::vector<int>) std::byte storage[sizeof(std::vector<int>)];
        ints.copy(storage, &source);
        auto* copied = std::launder(reinterpret_cast<std::vector<int>*>(storage));
        expect(*copied == source);
        ints.destroy(storage);
    };
}

void test_user_container_traits() {
    "a user ContainerTraits specialisation makes a custom container inspectable"_test = [] {
        RcStack stack{{4, 5, 6}};
        Ref ref = Ref::make(stack);

        expect(ref.type().kind == TypeKind::Sequence);
        expect(ref.type().name == "stack<int32>");
        expect(ref.size() == 3_ul);
        expect(!ref.resizable());           // no resize() in the traits
        expect(ref.path("[1]").as<int>() == 5_i);
    };
}

}

void run_reflection_containers_tests() {
    test_container_kinds_and_names();
    test_container_ops_describe_element_types();
    test_sequence_size_and_at();
    test_vector_resize_push_erase();
    test_array_is_fixed_size();
    test_vector_without_default_element();
    test_map_find_insert_erase();
    test_map_rejects_wrong_key_type();
    test_map_for_each_entry();
    test_optional();
    test_enum_integer_value();
    test_path_through_containers();
    test_path_through_optional();
    test_path_container_errors();
    test_path_on_root_container();
    test_nested_container_in_map();
    test_self_referencing_types();
    test_self_reference_through_map_and_optional();
    test_operations_on_the_wrong_kind();
    test_optional_without_default_value();
    test_path_key_without_default_constructor();
    test_path_key_constructor_throws();
    test_vector_bool_stays_opaque();
    test_copy_respects_element_copyability();
    test_user_container_traits();
}
