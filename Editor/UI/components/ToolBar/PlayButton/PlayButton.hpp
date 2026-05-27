#ifndef PROTO_UI_TOOLBAR_PLAYBUTTON_HPP
#define PROTO_UI_TOOLBAR_PLAYBUTTON_HPP

#include <map>
#include <vector>

#include "Registry.hpp"
#include "System/ISystem.hpp"
#include "Reflection/SceneSnapshot.hpp"

namespace Editor::UI::Components {

    class PlayButton {
    public:
        PlayButton() = default;
        ~PlayButton() = default;

        void render(ECS::registry &reg,
                    const std::map<ECS::System::ISystem *, bool> &enabledSystems);

        // Called every frame by App — ticks the game if running
        void tickGame(ECS::registry &reg,
                      const std::map<ECS::System::ISystem *, bool> &enabledSystems);

        [[nodiscard]] bool isGameRunning() const { return _gameRunning; }

    private:
        void startGame(ECS::registry &reg,
                       const std::map<ECS::System::ISystem *, bool> &enabledSystems);
        void stopGame(ECS::registry &reg);

        bool _gameRunning = false;
        std::vector<ECS::System::ISystem *> _applied;
        ECS::Reflection::SceneSnapshot _snapshot;
    };

}

#endif
