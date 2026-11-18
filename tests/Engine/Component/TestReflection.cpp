#include <boost/ut.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "ECS/ECS.hpp"
#include "ECS/Reflection/Reflection.hpp"
#include "Engine/Component/Physics/Buoyant.hpp"
#include "Engine/Component/Physics/CharacterController.hpp"
#include "Engine/Component/Physics/CharacterInput.hpp"
#include "Engine/Component/Physics/Collider.hpp"
#include "Engine/Component/Physics/Drag.hpp"
#include "Engine/Component/Physics/FluidVolume.hpp"
#include "Engine/Component/Physics/GravityAttractor.hpp"
#include "Engine/Component/Physics/RadialField.hpp"
#include "Engine/Component/Physics/RigidBody.hpp"
#include "Engine/Component/Physics/Spring.hpp"
#include "Engine/Component/Physics/SurfaceContact.hpp"
#include "Engine/Component/Transform.hpp"

using namespace boost::ut;
using namespace Engine;
using ECS::reflect::Ref;
using ECS::reflect::RegistryInspector;
using ECS::reflect::TypeInfo;
using ECS::reflect::TypeKind;
using ECS::reflect::TypeRegistry;

namespace {
const TypeInfo* typeNamed(std::string_view name) {
    return TypeRegistry::instance().find(name);
}

std::vector<std::string> fieldNames(const TypeInfo& type) {
    std::vector<std::string> names;
    for (const auto& field : type.fields) {
        names.push_back(field.name);
    }
    return names;
}

struct Expected {
    std::string_view name;
    std::vector<std::string> fields;
};

// What each component shows in the editor and saves in a level. A field
// added to a component without its E_FIELD, or removed, shows up here.
const std::vector<Expected>& expectedComponents() {
    static const std::vector<Expected> components{
        {"Transform", {"position", "rotation"}},
        {"RigidBody", {"mass", "gravityScale", "velocity", "angularVelocity", "fixedRotation"}},
        {"Collider", {"shape", "offset", "response", "restitution", "staticFriction", "kineticFriction", "layer", "mask"}},
        {"Buoyant", {"shape", "offset"}},
        {"CharacterController",
         {"maxSpeed", "acceleration", "deceleration", "fullGripFriction", "pushStrength", "airAcceleration",
          "airDeceleration", "jumpHeight", "jumpCutFactor", "fallGravityMultiplier", "maxFallSpeed",
          "groundSnapDistance", "coyoteTime", "jumpBufferTime", "maxGroundAngle", "mode"}},
        {"CharacterInput", {}},
        {"Drag", {"linear", "quadratic", "flow"}},
        {"FluidVolume", {"halfSize", "density", "drag"}},
        {"GravityAttractor", {"mass"}},
        {"RadialField", {"strength", "radius"}},
        {"Spring", {"anchor", "restLength", "stiffness", "damping", "bodyPoint", "otherPoint"}},
        {"SurfaceContact", {}},
    };
    return components;
}
}  // namespace

void run_component_reflection_tests() {
    "every component is reflected under its short name, with its designer fields"_test = [] {
        for (const Expected& component : expectedComponents()) {
            const TypeInfo* type = typeNamed(component.name);
            expect((type != nullptr) >> fatal) << "not registered:" << std::string(component.name);
            expect(type->kind == TypeKind::Struct) << std::string(component.name);
            expect(fieldNames(*type) == component.fields) << "fields of" << std::string(component.name);
        }
    };

    "Vector2f is a struct of two floats, as the editor's mouse expects"_test = [] {
        const TypeInfo* vector = typeNamed("Vector2f");
        expect((vector != nullptr) >> fatal);
        expect(fieldNames(*vector) == std::vector<std::string>{"x", "y"});
    };

    "a Collider's shape is a Variant of the three shapes, switched through the reflection"_test = [] {
        ECS::Registry world;
        auto inspector = world.inspect();
        const ECS::Entity entity = world.spawnEntity();
        Ref collider = inspector.add(entity, *typeNamed("Collider"));
        expect((collider.valid()) >> fatal);

        const Ref shape = collider.path("shape");
        expect((shape.is(ECS::reflect::TypeKind::Variant)) >> fatal);
        expect(shape.type().name == "variant<Collider::Box,Collider::Circle,Collider::Polygon>");
        expect(shape.alternativeIndex() == std::optional<std::size_t>(0)) << "a box by default";
        expect(collider.path("shape.halfSize.x").as<float>() == 0.5_f);

        shape.emplaceAlternative(1);
        collider.path("shape.radius").as<float>() = 2.f;
        const Component::Collider& stored = world.getComponent<Component::Collider>(entity);
        expect((std::holds_alternative<Component::Collider::Circle>(stored.shape)) >> fatal);
        expect(std::get<Component::Collider::Circle>(stored.shape).radius == 2._f);
        expect(typeNamed("Buoyant")->findField("shape")->type == &shape.type()) << "one TypeInfo for both";
    };

    "no name collides: every registration succeeded"_test = [] {
        expect(RegistryInspector::errors().empty());
    };

    "a component is added, read and written through the reflection"_test = [] {
        ECS::Registry world;
        auto inspector = world.inspect();
        const ECS::Entity entity = world.spawnEntity();

        Ref body = inspector.add(entity, *typeNamed("RigidBody"));
        expect((body.valid()) >> fatal);
        expect(body.path("mass").as<float>() == 1._f) << "default-constructed";
        body.path("mass").as<float>() = 4.f;
        body.path("velocity.x").as<float>() = 3.f;

        const Component::RigidBody& stored = world.getComponent<Component::RigidBody>(entity);
        expect(stored.mass == 4._f);
        expect(stored.velocity.x == 3._f);
        expect(stored.velocity.y == 0._f);
    };
}
