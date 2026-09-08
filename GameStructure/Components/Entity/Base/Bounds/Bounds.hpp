/*
    Created on: 3/2/26
    Filename: Bounds.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_BOUNDS_HPP
#define PROTO_BOUNDS_HPP
#include "SdlTypes.hpp"
#include "IComponent.hpp"
#include "ComponentRegistry.hpp"

namespace ECS::Entity {
    struct Bounds : public IComponent {
        float _width;
        float _height;
        Proto::Color _color;

        explicit Bounds(const float width = 100.0f, const float height = 100.0f, const Proto::Color &color = Proto::Color::White())
            : _width(width), _height(height), _color(color) {}

        std::string getName() override { return "Bounds"; }

        std::vector<ECS::Reflection::FieldDescriptor> getFields() override
        {
            return {
                { "width", ECS::Reflection::FieldType::Float, &_width },
                { "height", ECS::Reflection::FieldType::Float, &_height },
                { "color", ECS::Reflection::FieldType::Color, &_color }
            };
        }

        void fromJson(const nlohmann::json& j) override
        {
            _width  = j.value("width",  _width);
            _height = j.value("height", _height);
            if (j.contains("color")) {
                const auto& c = j["color"];
                _color.r = static_cast<uint8_t>(c.value("r", (int)_color.r));
                _color.g = static_cast<uint8_t>(c.value("g", (int)_color.g));
                _color.b = static_cast<uint8_t>(c.value("b", (int)_color.b));
                _color.a = static_cast<uint8_t>(c.value("a", (int)_color.a));
            }
        }
    };
}

REGISTER_COMPONENT(ECS::Entity::Bounds, "Bounds")

#endif //PROTO_BOUNDS_HPP
