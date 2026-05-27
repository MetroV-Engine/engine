/*
    Created on: 2/19/26
    Filename: IComponentFactory.hpp
    Description: ${DESCRIPTION}

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/


#ifndef PROTO_ICOMPONENTFACTORY_HPP
#define PROTO_ICOMPONENTFACTORY_HPP

#include "Entity.hpp"
#include "Registry.hpp"

class IComponentFactory {
public:
    virtual ~IComponentFactory() = default;

    // Allow to add more components generically from external code
    template<typename ComponentType, typename... Args>
    void create_component(registry& reg, const entity& e, Args&&... args) {
        reg.emplace_component<ComponentType>(e, std::forward<Args>(args)...);
    }
};

#endif //PROTO_ICOMPONENTFACTORY_HPP