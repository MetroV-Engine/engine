#include <boost/ut.hpp>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

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

    // ---- step

    "step before init returns 1 and updates nothing"_test = [] {
        Log log;
        Engine engine;
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));

        expect(engine.step(0.016) == 1);
        expect(a->updates == 0);
    };

    "step updates the scenes once with the given dt"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));

        expect(engine.step(0.25) == 0);

        expect(a->updates == 1);
        expect(a->lastDt == 0.25);
    };

    "step does not exit the scenes nor empty the stack"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        engine.step(0.016);
        engine.step(0.016);
        expect(!engine.scenes().empty());
        expect(log == Log{"A:onEnter", "A:update", "A:update"});
    };

    "step with no scene returns 0"_test = [] {
        Engine engine;
        engine.init();
        expect(engine.step(0.016) == 0);
    };

    "step accepts a dt of 0"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));
        expect(engine.step(0.0) == 0);
        expect(a->updates == 1);
    };

    "step with a negative dt returns 1 and updates nothing"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));
        expect(engine.step(-0.1) == 1);
        expect(a->updates == 0);
    };

    "step with a NaN dt returns 1 and updates nothing"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));
        expect(engine.step(std::numeric_limits<double>::quiet_NaN()) == 1);
        expect(a->updates == 0);
    };

    "step runs without run: a host can drive the frames itself"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(endsAfter(2, engine, log));
        int frames = 0;
        while (!engine.scenes().empty()) {
            engine.step(0.016);
            ++frames;
        }
        expect(frames == 2);
        expect(log == Log{"A:onEnter", "A:update", "A:update", "A:onExit"});
    };

    "step is not stopped by stop"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));
        engine.stop();
        expect(engine.step(0.016) == 0);
        expect(a->updates == 1);
    };

    // ---- shutdown

    "shutdown exits and destroys the scenes"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        log.clear();

        expect(engine.shutdown() == 0);

        expect(engine.scenes().empty());
        expect(log == Log{"A:onExit"});
    };

    "shutdown on a fresh engine returns 0"_test = [] {
        Engine engine;
        expect(engine.shutdown() == 0);
    };

    "shutdown can be called twice"_test = [] {
        Engine engine;
        engine.init();
        expect(engine.shutdown() == 0);
        expect(engine.shutdown() == 0);
    };

    "run and step need init again after shutdown"_test = [] {
        Engine engine;
        engine.init();
        engine.shutdown();
        expect(engine.run() == 1);
        expect(engine.step(0.016) == 1);
        engine.init();
        expect(engine.step(0.016) == 0);
    };

    "shutdown destroys the services after the scenes"_test = [] {
        struct Probe {
            explicit Probe(Log& l) : log(l) {}
            ~Probe() { log.push_back("service:destroyed"); }
            Log& log;
        };
        Log log;
        Engine engine;
        engine.init();
        engine.addService<Probe>(log);
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        log.clear();

        engine.shutdown();

        expect(log == Log{"A:onExit", "service:destroyed"});
        expect(engine.services().size() == std::size_t{0});
    };

    // ---- services

    "a service added before a scene is pushed is injected into it"_test = [] {
        struct Gravity { double value; };
        Log log;
        Engine engine;
        engine.addService<Gravity>(Gravity{9.8});
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        expect(getService<Gravity>(engine.scenes().top()->registry()).value == 9.8);
    };

    "a service added after a scene is pushed is injected into it"_test = [] {
        struct Gravity { double value; };
        Log log;
        Engine engine;
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        engine.addService<Gravity>(Gravity{1.5});
        expect(getService<Gravity>(engine.scenes().top()->registry()).value == 1.5);
    };

    "every scene of the stack shares the same service"_test = [] {
        struct Counter { int value = 0; };
        Log log;
        Engine engine;
        Counter& counter = engine.addService<Counter>();
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        Scene* a = engine.scenes().top();
        engine.scenes().push(std::make_unique<RecordingScene>("B", log));

        getService<Counter>(a->registry()).value = 5;

        expect(&getService<Counter>(engine.scenes().top()->registry()) == &counter);
        expect(counter.value == 5);
    };

    "a service survives the scene that used it"_test = [] {
        struct Counter { int value = 0; };
        Log log;
        Engine engine;
        engine.addService<Counter>().value = 3;
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        getService<Counter>(engine.scenes().top()->registry()).value = 8;
        engine.scenes().pop();

        engine.scenes().push(std::make_unique<RecordingScene>("B", log));

        expect(getService<Counter>(engine.scenes().top()->registry()).value == 8);
    };

    "a system reads the service through its registry during step"_test = [] {
        struct Counter { int value = 0; };
        struct Bump : ECS::ISystem {
            void update(ECS::Registry& reg, double) override { ++getService<Counter>(reg).value; }
            const std::string name() const override { return "Bump"; }
        };
        Log log;
        Engine engine;
        engine.init();
        Counter& counter = engine.addService<Counter>();
        auto scene = std::make_unique<RecordingScene>("A", log);
        scene->systems().addSystem<Bump>(scene->registry());
        engine.scenes().push(std::move(scene));

        engine.step(0.016);
        engine.step(0.016);

        expect(counter.value == 2);
    };

    "a scene created by id gets the services"_test = [] {
        struct Gravity { double value; };
        Log log;
        Engine engine;
        engine.addService<Gravity>(Gravity{2.0});
        engine.scenes().registry().add("a", [&log] { return std::make_unique<RecordingScene>("A", log); });
        expect(engine.scenes().push("a"));
        expect(getService<Gravity>(engine.scenes().top()->registry()).value == 2.0);
    };

    "a scene without the service throws when asking for it"_test = [] {
        struct Gravity { double value; };
        Log log;
        Engine engine;
        engine.scenes().push(std::make_unique<RecordingScene>("A", log));
        expect(throws<std::out_of_range>([&] { (void)getService<Gravity>(engine.scenes().top()->registry()); }));
    };

    "services returns the same object on every call"_test = [] {
        Engine engine;
        expect(&engine.services() == &engine.services());
    };

    "a service can be used by a scene in onExit"_test = [] {
        struct Counter { int value = 7; };
        struct Exiting : Scene {
            explicit Exiting(int& seen) : Scene("Exiting"), _seen(seen) {}
            void onExit() override { _seen = getService<Counter>(registry()).value; }
            int& _seen;
        };
        int seen = 0;
        Engine engine;
        engine.addService<Counter>();
        engine.scenes().push(std::make_unique<Exiting>(seen));
        engine.shutdown();
        expect(seen == 7);
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

    // ---- frame rate: setTargetFrameRate / setMaxFrameTime

    "a new engine runs at 60 frames per second and caps dt at 0.25 s"_test = [] {
        const Engine engine;
        expect(engine.targetFrameRate() == 60.0);
        expect(engine.maxFrameTime() == 0.25);
    };

    "setTargetFrameRate accepts a positive rate or 0"_test = [] {
        Engine engine;
        expect(engine.setTargetFrameRate(60.0));
        expect(engine.targetFrameRate() == 60.0);
        expect(engine.setTargetFrameRate(0.0));
        expect(engine.targetFrameRate() == 0.0);
    };

    "setTargetFrameRate refuses a negative, NaN or infinite rate and keeps the previous one"_test = [] {
        Engine engine;
        engine.setTargetFrameRate(30.0);
        expect(!engine.setTargetFrameRate(-1.0));
        expect(!engine.setTargetFrameRate(std::numeric_limits<double>::quiet_NaN()));
        expect(!engine.setTargetFrameRate(std::numeric_limits<double>::infinity()));
        expect(engine.targetFrameRate() == 30.0);
    };

    "setMaxFrameTime accepts a positive duration or 0"_test = [] {
        Engine engine;
        expect(engine.setMaxFrameTime(0.1));
        expect(engine.maxFrameTime() == 0.1);
        expect(engine.setMaxFrameTime(0.0));
        expect(engine.maxFrameTime() == 0.0);
    };

    "setMaxFrameTime refuses a negative, NaN or infinite duration and keeps the previous one"_test = [] {
        Engine engine;
        engine.setMaxFrameTime(0.5);
        expect(!engine.setMaxFrameTime(-0.1));
        expect(!engine.setMaxFrameTime(std::numeric_limits<double>::quiet_NaN()));
        expect(!engine.setMaxFrameTime(std::numeric_limits<double>::infinity()));
        expect(engine.maxFrameTime() == 0.5);
    };

    "run with a target frame rate sleeps between frames"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.setTargetFrameRate(100.0);   // one frame every 10 ms
        engine.scenes().push(endsAfter(6, engine, log));

        const auto start = std::chrono::steady_clock::now();
        expect(engine.run() == 0);
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        expect(elapsed >= 0.045);   // 5 sleeps of 10 ms between the 6 frames, minus some slack
        expect(elapsed < 2.0);
    };

    "run with a target frame rate gives frames a dt close to the period"_test = [] {
        Engine engine;
        engine.init();
        engine.setTargetFrameRate(50.0);    // 20 ms
        Log log;
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        double smallestLaterDt = 1.0;
        scene->onUpdate = [&](double dt) {
            if (a->updates > 1)
                smallestLaterDt = std::min(smallestLaterDt, dt);
            if (a->updates >= 4)
                engine.scenes().pop();
        };
        engine.scenes().push(std::move(scene));

        engine.run();

        expect(smallestLaterDt >= 0.015);
    };

    "run caps a long frame's dt at maxFrameTime"_test = [] {
        Engine engine;
        engine.init();
        engine.setMaxFrameTime(0.02);
        Log log;
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        double secondDt = -1.0;
        scene->onUpdate = [&](double dt) {
            if (a->updates == 1)
                std::this_thread::sleep_for(std::chrono::milliseconds(60));   // a slow frame
            if (a->updates == 2) {
                secondDt = dt;
                engine.scenes().pop();
            }
        };
        engine.scenes().push(std::move(scene));

        engine.run();

        expect(secondDt >= 0.0);
        expect(secondDt <= 0.02);
    };

    "run does not cap dt when maxFrameTime is 0"_test = [] {
        Engine engine;
        engine.init();
        engine.setMaxFrameTime(0.0);
        Log log;
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        double secondDt = -1.0;
        scene->onUpdate = [&](double dt) {
            if (a->updates == 1)
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
            if (a->updates == 2) {
                secondDt = dt;
                engine.scenes().pop();
            }
        };
        engine.scenes().push(std::move(scene));

        engine.run();

        expect(secondDt >= 0.05);
    };

    "step neither sleeps nor caps dt: the host drives the frames"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.setTargetFrameRate(1.0);     // one frame per second, if step slept
        engine.setMaxFrameTime(0.1);
        auto scene = std::make_unique<RecordingScene>("A", log);
        RecordingScene* a = scene.get();
        engine.scenes().push(std::move(scene));

        const auto start = std::chrono::steady_clock::now();
        engine.step(1.5);
        engine.step(1.5);
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        expect(a->lastDt == 1.5);
        expect(elapsed < 0.5);
    };

    "stop with a target frame rate ends run without waiting for another frame"_test = [] {
        Log log;
        Engine engine;
        engine.init();
        engine.setTargetFrameRate(1.0);     // a 1 s sleep, if run slept after stop
        auto scene = std::make_unique<RecordingScene>("A", log);
        scene->onUpdate = [&engine](double) { engine.stop(); };
        engine.scenes().push(std::move(scene));

        const auto start = std::chrono::steady_clock::now();
        engine.run();
        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

        expect(elapsed < 0.5);
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
