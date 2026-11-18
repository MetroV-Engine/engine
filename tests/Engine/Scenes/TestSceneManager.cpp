#include <boost/ut.hpp>

#include <memory>
#include <string>

#include "Engine/Scenes/SceneManager.hpp"
#include "Support/RecordingScene.hpp"

using namespace boost::ut;

namespace {
    std::unique_ptr<RecordingScene> make(const std::string& name, Log& log) {
        return std::make_unique<RecordingScene>(name, log);
    }
}

void run_scene_manager_tests() {
    // ---- empty / top

    "a new manager is empty and has no top scene"_test = [] {
        SceneManager scenes;
        expect(scenes.empty());
        expect(scenes.top() == nullptr);
    };

    "top returns the last pushed scene"_test = [] {
        Log log;
        SceneManager scenes;
        auto a = make("A", log);
        auto b = make("B", log);
        Scene* pa = a.get();
        Scene* pb = b.get();
        scenes.push(std::move(a));
        expect(scenes.top() == pa);
        scenes.push(std::move(b));
        expect(scenes.top() == pb);
        expect(!scenes.empty());
    };

    // ---- push

    "push calls onEnter on the new scene"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        expect(log == Log{"A:onEnter"});
    };

    "push pauses the previous top scene before entering the new one"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        scenes.push(make("B", log));
        expect(log == Log{"A:onEnter", "A:onPause", "B:onEnter"});
    };

    "push of a null scene is ignored"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(nullptr);
        expect(scenes.empty());

        scenes.push(make("A", log));
        scenes.push(nullptr);
        expect(log == Log{"A:onEnter"}); // A was not paused
        expect(scenes.top()->name() == std::string("A"));
    };

    // ---- pop

    "pop exits the top scene and resumes the one below"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        scenes.push(make("B", log));
        log.clear();

        scenes.pop();

        expect(log == Log{"B:onExit", "A:onResume"});
        expect(scenes.top()->name() == std::string("A"));
    };

    "pop of the last scene empties the manager without resuming anything"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        log.clear();

        scenes.pop();

        expect(log == Log{"A:onExit"});
        expect(scenes.empty());
        expect(scenes.top() == nullptr);
    };

    "pop on an empty manager does nothing"_test = [] {
        SceneManager scenes;
        scenes.pop();
        expect(scenes.empty());
    };

    "pop destroys the scene"_test = [] {
        SceneManager scenes;
        bool destroyed = false;
        struct Probe : Scene {
            explicit Probe(bool& d) : Scene("Probe"), _d(d) {}
            ~Probe() override { _d = true; }
            bool& _d;
        };
        scenes.push(std::make_unique<Probe>(destroyed));
        expect(!destroyed);
        scenes.pop();
        expect(destroyed);
    };

    // ---- clear

    "clear exits every scene from the top down, without resuming any"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        scenes.push(make("B", log));
        scenes.push(make("C", log));
        log.clear();

        scenes.clear();

        expect(log == Log{"C:onExit", "B:onExit", "A:onExit"});
        expect(scenes.empty());
    };

    "clear on an empty manager does nothing"_test = [] {
        SceneManager scenes;
        scenes.clear();
        expect(scenes.empty());
    };

    "clear can be called twice"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        scenes.clear();
        log.clear();
        scenes.clear();
        expect(log.empty());
    };

    "a manager can be reused after clear"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        scenes.clear();
        scenes.push(make("B", log));
        expect(scenes.top()->name() == std::string("B"));
    };

    "the destructor exits the remaining scenes"_test = [] {
        Log log;
        {
            SceneManager scenes;
            scenes.push(make("A", log));
            scenes.push(make("B", log));
            log.clear();
        }
        expect(log == Log{"B:onExit", "A:onExit"});
    };

    // ---- update

    "update on an empty manager does nothing"_test = [] {
        SceneManager scenes;
        scenes.update(1.0);
        expect(scenes.empty());
    };

    "update runs the only scene with dt"_test = [] {
        Log log;
        SceneManager scenes;
        auto scene = make("A", log);
        RecordingScene* a = scene.get();
        scenes.push(std::move(scene));

        scenes.update(0.5);

        expect(a->updates == 1);
        expect(a->lastDt == 0.5);
    };

    "update skips the covered scenes by default"_test = [] {
        Log log;
        SceneManager scenes;
        auto bottom = make("A", log);
        RecordingScene* a = bottom.get();
        auto top = make("B", log);
        RecordingScene* b = top.get();
        scenes.push(std::move(bottom));
        scenes.push(std::move(top));

        scenes.update(1.0);

        expect(a->updates == 0);
        expect(b->updates == 1);
    };

    "update runs a covered scene that asks for it, below the top one"_test = [] {
        Log log;
        SceneManager scenes;
        auto bottom = make("A", log);
        bottom->updatesWhenCovered = true;
        RecordingScene* a = bottom.get();
        scenes.push(std::move(bottom));
        scenes.push(make("B", log));
        log.clear();

        scenes.update(0.1);

        expect(a->updates == 1);
        expect(a->lastDt == 0.1);
        expect(log == Log{"A:update", "B:update"}); // bottom to top
    };

    "update only runs the covered scenes that ask for it"_test = [] {
        Log log;
        SceneManager scenes;
        auto a = make("A", log);
        auto b = make("B", log);
        b->updatesWhenCovered = true;
        scenes.push(std::move(a));
        scenes.push(std::move(b));
        scenes.push(make("C", log));
        log.clear();

        scenes.update(1.0);

        expect(log == Log{"B:update", "C:update"});
    };

    "update of the manager accepts a zero dt"_test = [] {
        Log log;
        SceneManager scenes;
        auto scene = make("A", log);
        RecordingScene* a = scene.get();
        scenes.push(std::move(scene));
        scenes.update(0.0);
        expect(a->updates == 1);
        expect(a->lastDt == 0.0);
    };

    "a scene can pop itself during update"_test = [] {
        Log log;
        SceneManager scenes;
        auto scene = make("A", log);
        scene->onUpdate = [&scenes](double) { scenes.pop(); };
        scenes.push(std::move(scene));

        scenes.update(1.0);

        expect(scenes.empty());
        expect(log == Log{"A:onEnter", "A:update", "A:onExit"});
    };

    "a scene popping itself resumes the one below"_test = [] {
        Log log;
        SceneManager scenes;
        scenes.push(make("A", log));
        auto top = make("B", log);
        top->onUpdate = [&scenes](double) { scenes.pop(); };
        scenes.push(std::move(top));
        log.clear();

        scenes.update(1.0);

        expect(log == Log{"B:update", "B:onExit", "A:onResume"});
        expect(scenes.top()->name() == std::string("A"));
    };

    "a scene can push another scene during update"_test = [] {
        Log log;
        SceneManager scenes;
        auto scene = make("A", log);
        scene->onUpdate = [&scenes, &log](double) {
            scenes.push(std::make_unique<RecordingScene>("B", log));
            scenes.top()->onExit(); // exercises top() while the vector grew
        };
        scenes.push(std::move(scene));

        scenes.update(1.0);

        expect(scenes.top()->name() == std::string("B"));
    };
}
