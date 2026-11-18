#include <boost/ut.hpp>

#include <memory>
#include <string>

#include "Engine/Engine.hpp"
#include "Support/RecordingScene.hpp"

using namespace boost::ut;

namespace {
    // A scene that pops itself after `frames` updates.
    std::unique_ptr<RecordingScene> endsAfter(int frames, Engine& engine, Log& log, const std::string& name = "A") {
        auto scene = std::make_unique<RecordingScene>(name, log);
        RecordingScene* raw = scene.get();
        scene->onUpdate = [&engine, raw, frames](double) {
            if (raw->updates >= frames)
                engine.scenes().pop(); // destroys the scene: last thing done
        };
        return scene;
    }
}

void run_engine_tests() {
    // ---- init

    "init returns 0"_test = [] {
        Engine engine;
        expect(engine.init() == 0);
    };

    "init can be called twice"_test = [] {
        Engine engine;
        expect(engine.init() == 0);
        expect(engine.init() == 0);
    };

    // ---- scenes

    "scenes returns the same manager on every call"_test = [] {
        Engine engine;
        expect(&engine.scenes() == &engine.scenes());
    };

    "a new engine has no scene"_test = [] {
        Engine engine;
        expect(engine.scenes().empty());
    };

    "scenes pushed through the engine are kept by it"_test = [] {
        Log log;
        Engine engine;
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        expect(!engine.scenes().empty());
        expect(engine.scenes().top()->name() == std::string("A"));
    };

    // ---- run

    "run before init returns 1 and updates nothing"_test = [] {
        Log log;
        Engine engine;
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));

        expect(engine.run() == 1);
        expect(a->updates == 0);
        expect(!engine.scenes().empty()); // left untouched
    };

    "run with no scene returns 0 at once"_test = [] {
        Engine engine;
        engine.init();
        expect(engine.run() == 0);
    };

    "run updates the scene every frame until it is gone"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = endsAfter(3, engine, log);
        engine.scenes().push(std::move(scene));

        expect(engine.run() == 0);

        expect(engine.scenes().empty());
        expect(log == Log{"A:onEnter", "A:update", "A:update", "A:update", "A:onExit"});
    };

    "run gives every scene a non-negative dt"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        bool allNonNegative = true;
        scene->onUpdate = [&](double dt) {
            allNonNegative = allNonNegative && dt >= 0.0;
            if (a->updates >= 5)
                engine.scenes().pop();
        };
        engine.scenes().push(std::move(scene));

        engine.run();

        expect(allNonNegative);
    };

    "run follows the scene stack until it is empty"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(endsAfter(1, engine, log, "A"));
        engine.scenes().push(endsAfter(1, engine, log, "B"));

        expect(engine.run() == 0);

        expect(engine.scenes().empty());
        expect(log == Log{"A:onEnter", "A:onPause", "B:onEnter", "B:update", "B:onExit", "A:onResume",
                          "A:update", "A:onExit"});
    };

    "run ends when a scene pushes nothing and pops the last one"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(endsAfter(1, engine, log));
        expect(engine.run() == 0);
        expect(engine.scenes().empty());
    };

    // ---- stop

    "stop returns 0"_test = [] {
        Engine engine;
        expect(engine.stop() == 0);
    };

    "stop from a scene ends run and exits the remaining scenes"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        scene->onUpdate = [&engine](double) { engine.stop(); };
        engine.scenes().push(std::move(scene));

        expect(engine.run() == 0);

        expect(engine.scenes().empty());
        expect(log == Log{"A:onEnter", "A:update", "A:onExit"});
    };

    "stop ends the loop after the current frame"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        scene->onUpdate = [&engine](double) { engine.stop(); };
        engine.scenes().push(std::move(scene));
        // The scene is gone after run(): read the count from the log.
        engine.run();
        int updates = 0;
        for (const std::string& line : log)
            if (line == "A:update")
                ++updates;
        expect(updates == 1);
    };

    "stop before run does not prevent run from running"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.stop();
        engine.scenes().push(endsAfter(2, engine, log));

        expect(engine.run() == 0);

        expect(log == Log{"A:onEnter", "A:update", "A:update", "A:onExit"});
    };

    "stop can be called twice"_test = [] {
        Engine engine;
        expect(engine.stop() == 0);
        expect(engine.stop() == 0);
    };

    // ---- run again

    "run can be called again with new scenes"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(endsAfter(1, engine, log, "A"));
        expect(engine.run() == 0);

        engine.scenes().push(endsAfter(1, engine, log, "B"));
        expect(engine.run() == 0);

        expect(log == Log{"A:onEnter", "A:update", "A:onExit", "B:onEnter", "B:update", "B:onExit"});
    };

    "run works after a failed run once init is called"_test = [] {
        Log log;
        Engine engine;
        engine.scenes().push(endsAfter(1, engine, log));
        expect(engine.run() == 1);

        engine.init();
        expect(engine.run() == 0);
        expect(engine.scenes().empty());
    };

    // ---- destruction

    "destroying the engine exits the scenes still on the stack"_test = [] {
        Log log;
        {
            Engine engine;
            engine.scenes().push(std::make_unique<RecordingScene>("A", log));
            log.clear();
        }
        expect(log == Log{"A:onExit"});
    };
}
