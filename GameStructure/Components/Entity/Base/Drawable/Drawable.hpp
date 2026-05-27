/*
** EPITECH PROJECT, 2026
** MetroV-prototype
** File description:
** Drawable
*/

#ifndef DRAWABLE_HPP_
#define DRAWABLE_HPP_

#include "IComponent.hpp"
#include "ComponentRegistry.hpp"
#include "Reflection/FieldDescriptor.hpp"

namespace ECS::Entity {
    struct Drawable : public IComponent {
        bool _drawable;
        explicit Drawable(const bool drawable = true)
            : _drawable(drawable) {}
    
        std::string getName() override
        {
            return "Drawable";
        }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {   
                { "drawable", ECS::Reflection::FieldType::Bool, &_drawable }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _drawable = j.value("drawable", _drawable);
        }
    };
}

REGISTER_COMPONENT(ECS::Entity::Drawable, "Drawable")


#endif /* !DRAWABLE_HPP_ */
