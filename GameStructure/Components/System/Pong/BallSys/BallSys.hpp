#ifndef PROTO_BALLSYS_HPP
#define PROTO_BALLSYS_HPP
#include "System/ISystem.hpp"
namespace ECS::System {
    class BallSys : public ISystem {
        public:
            BallSys() = default;
            void update(registry &r, float deltaTime, SDL_Window *win) override;
            [[nodiscard]] const char* getName() const override {return "BallSys";};
        private:
            void checkWalls(registry &r, SDL_Window *win);
            void checkCollisionWithPad(registry &r);
            void checkOutOfBounds(registry &r, SDL_Window *win);
    };
}
#endif
