/*
    Created on: 3/3/26
    Filename: FieldDescriptor.hpp
    Description: Compile-time component metadata reflection

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_FIELDDESCRIPTOR_HPP
#define PROTO_FIELDDESCRIPTOR_HPP

#include <string>
#include <variant>
#include <functional>
#include <unordered_map>
#include <map>
#include "SdlTypes.hpp"
#include "Entity/Moves.hpp"

namespace ECS::Reflection {

    enum class FieldType {
        Float,
        Int,
        Bool,
        Vec2f,
        String,
        KeyMapLayout,
        Color
    };

    using FieldValue = std::variant<
        float*,
        int*,
        bool*,
        Proto::Vec2f*,
        std::string*,
        std::map<ECS::Entity::Moves, Proto::Key>*,
        Proto::Color*
    >;

    struct FieldDescriptor {
        std::string name;
        FieldType   type;
        FieldValue  valuePtr;
    };

} // ECS::Reflection

namespace std {
    template<>
    struct hash<ECS::Reflection::FieldType> {
        std::size_t operator()(const ECS::Reflection::FieldType& type) const noexcept {
            return std::hash<int>{}(static_cast<int>(type));
        }
    };
}

#endif //PROTO_FIELDDESCRIPTOR_HPP
