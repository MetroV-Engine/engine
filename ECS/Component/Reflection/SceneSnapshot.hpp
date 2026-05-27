/*
    Created on: 3/6/26
    Filename: SceneSnapshot.hpp

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_SCENESNAPSHOT_HPP
#define PROTO_SCENESNAPSHOT_HPP

#include <unordered_map>
#include <variant>
#include <cstddef>
#include "SdlTypes.hpp"
#include "FieldDescriptor.hpp"
#include "Entity/Moves.hpp"
#include "IComponent.hpp"
#include "Registry.hpp"

namespace ECS::Reflection {

    using FieldSnapshot = std::variant<
        float,
        int,
        bool,
        Proto::Vec2f,
        std::string,
        std::map<ECS::Entity::Moves, Proto::Key>,
        Proto::Color
    >;

    class SceneSnapshot {
    public:
        void capture(ECS::registry &reg);
        void restore(ECS::registry &reg);

    private:
        struct FieldKey {
            std::size_t entityId;
            std::string componentName;
            std::string fieldName;

            bool operator==(const FieldKey &o) const noexcept;
        };

        struct FieldKeyHash {
            std::size_t operator()(const FieldKey &k) const noexcept;
        };

        std::unordered_map<FieldKey, FieldSnapshot, FieldKeyHash> _data;

        static FieldSnapshot snapshotValue(const FieldDescriptor &fd);
        static void          restoreValue(const FieldDescriptor &fd, const FieldSnapshot &snap);
    };

} // namespace ECS::Reflection

#endif //PROTO_SCENESNAPSHOT_HPP
