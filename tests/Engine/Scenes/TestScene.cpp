#include <boost/ut.hpp>

#include <string>
#include <type_traits>

#include "Engine/Scenes/Scene.hpp"
#include "Support/RecordingScene.hpp"

using namespace boost::ut;

namespace {
    static_assert(!std::is_copy_constructible_v<Scene>, "a scene owns a world: not copyable");
    static_assert(!std::is_copy_assignable_v<Scene>, "a scene owns a world: not copyable");

    // Records the dt it receives and the registry it runs on.
    class ProbeSystem : public ECS::ISystem {
        public:
            ProbeSystem(double& dt, ECS::Registry*& seen, int& calls) : _dt(dt), _seen(seen), _calls(calls) {}

            void update(ECS::Registry& reg, double dt) override {
                _dt = dt;
                _seen = &reg;
                ++_calls;
            }

            const std::string name() const override { return "ProbeSystem"; }

        private:
            double& _dt;
            ECS::Registry*& _seen;
            int& _calls;
    };

    struct DestructionFlag : Scene {
        explicit DestructionFlag(bool& destroyed) : Scene("Flag"), _destroyed(destroyed) {}
        ~DestructionFlag() override { _destroyed = true; }
        bool& _destroyed;
    };
}

void run_scene_tests() {
    "name defaults to Scene"_test = [] {
        Scene scene;
        expect(scene.name() == std::string("Scene"));
    };

    "name returns the given name"_test = [] {
        Scene scene("Level 1");
        expect(scene.name() == std::string("Level 1"));
    };

    "name accepts an empty string"_test = [] {
        Scene scene("");
        expect(scene.name().empty());
    };

    "registry returns the same registry on every call"_test = [] {
        Scene scene;
        expect(&scene.registry() == &scene.registry());
    };

    "each scene owns its own world"_test = [] {
        Scene a("A");
        Scene b("B");
        a.registry().spawnEntity();
        expect(&a.registry() != &b.registry());
        expect(a.registry().entityCount() == std::size_t{1});
        expect(b.registry().entityCount() == std::size_t{0});
    };

    "systems returns the same manager on every call"_test = [] {
        Scene scene;
        expect(&scene.systems() == &scene.systems());
    };

    "update runs the systems of the scene on its own registry with dt"_test = [] {
        Scene scene;
        double dt = -1.0;
        ECS::Registry* seen = nullptr;
        int calls = 0;
        scene.systems().addSystem<ProbeSystem>(scene.registry(), dt, seen, calls);

        scene.update(0.25);

        expect(calls == 1);
        expect(dt == 0.25);
        expect(seen == &scene.registry());
    };

    "update does not run the systems of another scene"_test = [] {
        Scene a("A");
        Scene b("B");
        double dt = -1.0;
        ECS::Registry* seen = nullptr;
        int calls = 0;
        a.systems().addSystem<ProbeSystem>(a.registry(), dt, seen, calls);

        b.update(1.0);

        expect(calls == 0);
    };

    "update without a system does nothing"_test = [] {
        Scene scene;
        scene.registry().spawnEntity();
        scene.update(1.0);
        expect(scene.registry().entityCount() == std::size_t{1});
    };

    "update accepts a zero dt"_test = [] {
        Scene scene;
        double dt = -1.0;
        ECS::Registry* seen = nullptr;
        int calls = 0;
        scene.systems().addSystem<ProbeSystem>(scene.registry(), dt, seen, calls);

        scene.update(0.0);

        expect(calls == 1);
        expect(dt == 0.0);
    };

    "lifecycle hooks do nothing by default"_test = [] {
        Scene scene;
        scene.registry().spawnEntity();
        scene.onEnter();
        scene.onPause();
        scene.onResume();
        scene.onExit();
        expect(scene.registry().entityCount() == std::size_t{1});
        expect(scene.name() == std::string("Scene"));
    };

    "a derived scene can override update and the lifecycle hooks"_test = [] {
        Log log;
        RecordingScene scene("S", log);
        Scene& base = scene;
        base.onEnter();
        base.update(0.5);
        base.onPause();
        base.onResume();
        base.onExit();
        expect(log == Log{"S:onEnter", "S:update", "S:onPause", "S:onResume", "S:onExit"});
        expect(scene.lastDt == 0.5);
    };

    "updatesWhenCovered is false by default"_test = [] {
        Scene scene;
        expect(!scene.updatesWhenCovered);
    };

    "updatesWhenCovered can be enabled"_test = [] {
        Scene scene;
        scene.updatesWhenCovered = true;
        expect(scene.updatesWhenCovered);
    };

    "destroying through a base pointer runs the derived destructor"_test = [] {
        bool destroyed = false;
        Scene* scene = new DestructionFlag(destroyed);
        delete scene;
        expect(destroyed);
    };
}
