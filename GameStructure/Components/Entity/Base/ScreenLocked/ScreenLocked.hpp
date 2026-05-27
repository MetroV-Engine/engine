#ifndef PROTO_SCREENLOCKED_HPP
#define PROTO_SCREENLOCKED_HPP

#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "Reflection/FieldDescriptor.hpp"

namespace ECS::Entity {
    struct ScreenLocked : public IComponent {
        bool _locked;
        explicit ScreenLocked(bool locked = true) : _locked(locked) {}

        std::string getName() override { return "ScreenLocked"; }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override {
            return {{ "locked", ECS::Reflection::FieldType::Bool, &_locked }};
        }

        void fromJson(const nlohmann::json& j) override {
            _locked = j.value("locked", _locked);
        }
    };
}

REGISTER_COMPONENT(ECS::Entity::ScreenLocked, "ScreenLocked")

#endif
