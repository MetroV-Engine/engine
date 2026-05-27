#ifndef PROTO_NATIVESCRIPTSYSTEM_HPP
#define PROTO_NATIVESCRIPTSYSTEM_HPP
#include "System/ISystem.hpp"
namespace ECS::System {
    class NativeScriptSystem : public ISystem {
    public:
        void update(registry &r, float dt, SDL_Window *win) override;
        [[nodiscard]] const char *getName() const override { return "NativeScriptSystem"; }
    };
}
#endif
