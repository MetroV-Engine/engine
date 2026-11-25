#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "Engine/Scenes/Scene.hpp"

// A scene that records what the SceneManager does to it, as "<name>:<event>"
// lines in a log shared by every scene of a test.
class RecordingScene : public Scene {
    public:
        RecordingScene(std::string name, std::vector<std::string>& log)
            : Scene(name), _log(log), _label(std::move(name)) {}

        // Called at the end of update(): a test makes the scene pop itself,
        // stop the engine... from there. Must be the last thing update() does,
        // since popping destroys the scene.
        std::function<void(double)> onUpdate;
        int updates = 0;
        double lastDt = -1.0;

        void update(double dt) override
        {
            ++updates;
            lastDt = dt;
            _log.push_back(_label + ":update");
            Scene::update(dt);
            if (onUpdate)
                onUpdate(dt);
        }

        void onEnter() override { _log.push_back(_label + ":onEnter"); }
        void onExit() override { _log.push_back(_label + ":onExit"); }
        void onPause() override { _log.push_back(_label + ":onPause"); }
        void onResume() override { _log.push_back(_label + ":onResume"); }

    private:
        std::vector<std::string>& _log;
        std::string _label;
};

using Log = std::vector<std::string>;
