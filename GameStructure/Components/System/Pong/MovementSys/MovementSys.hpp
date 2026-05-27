#ifndef PROTO_MOVEMENTSYS_HPP
#define PROTO_MOVEMENTSYS_HPP
#include "System/ISystem.hpp"
namespace ECS::System {
    class MovementSys : public ISystem {
        void update(registry &r, float dt, SDL_Window *win) override;
        [[nodiscard]] const char* getName() const override {return "MovementSys";};
    };
}
#endif
