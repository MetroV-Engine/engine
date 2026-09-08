#ifndef PROTO_CONTROLSSYS_HPP
#define PROTO_CONTROLSSYS_HPP
#include "System/ISystem.hpp"
namespace ECS::System {
    class ControlsSys : public ISystem {
        public:
            ControlsSys() = default;
            void update(registry &r, float deltaTime, SDL_Window *win) override;
            [[nodiscard]] const char* getName() const override {return "ControlsSys";};
        private:
            void checkPlayer1(registry &r, SDL_Window *win);
    };
}
#endif
