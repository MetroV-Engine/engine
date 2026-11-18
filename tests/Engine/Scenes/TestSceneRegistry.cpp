#include <boost/ut.hpp>

#include <memory>
#include <string>

#include "Engine/Scenes/SceneRegistry.hpp"
#include "Support/RecordingScene.hpp"

using namespace boost::ut;

namespace {
    SceneRegistry::Factory named(const std::string& name, Log& log) {
        return [name, &log] { return std::make_unique<RecordingScene>(name, log); };
    }
}

void run_scene_registry_tests() {
    // ---- add

    "add registers a factory"_test = [] {
        Log log;
        SceneRegistry registry;
        expect(registry.add("menu", named("Menu", log)));
        expect(registry.has("menu"));
        expect(registry.size() == std::size_t{1});
    };

    "add accepts ids that look like paths"_test = [] {
        Log log;
        SceneRegistry registry;
        expect(registry.add("levels/level2", named("L2", log)));
        expect(registry.has("levels/level2"));
    };

    "add refuses an empty id"_test = [] {
        Log log;
        SceneRegistry registry;
        expect(!registry.add("", named("A", log)));
        expect(registry.size() == std::size_t{0});
    };

    "add refuses an empty factory"_test = [] {
        SceneRegistry registry;
        expect(!registry.add("menu", SceneRegistry::Factory{}));
        expect(!registry.has("menu"));
    };

    "add refuses an id already taken and keeps the first factory"_test = [] {
        Log log;
        SceneRegistry registry;
        expect(registry.add("menu", named("First", log)));
        expect(!registry.add("menu", named("Second", log)));
        expect(registry.create("menu")->name() == std::string("First"));
        expect(registry.size() == std::size_t{1});
    };

    "ids are case sensitive"_test = [] {
        Log log;
        SceneRegistry registry;
        registry.add("Menu", named("A", log));
        expect(!registry.has("menu"));
    };

    // ---- has / size

    "a new registry is empty"_test = [] {
        SceneRegistry registry;
        expect(registry.size() == std::size_t{0});
        expect(!registry.has("anything"));
        expect(!registry.has(""));
    };

    // ---- remove

    "remove unregisters an id"_test = [] {
        Log log;
        SceneRegistry registry;
        registry.add("menu", named("A", log));
        expect(registry.remove("menu"));
        expect(!registry.has("menu"));
        expect(registry.create("menu") == nullptr);
    };

    "remove of an unknown id returns false"_test = [] {
        SceneRegistry registry;
        expect(!registry.remove("menu"));
        expect(!registry.remove(""));
    };

    "an id can be added again after remove"_test = [] {
        Log log;
        SceneRegistry registry;
        registry.add("menu", named("Old", log));
        registry.remove("menu");
        expect(registry.add("menu", named("New", log)));
        expect(registry.create("menu")->name() == std::string("New"));
    };

    // ---- create

    "create builds a new scene on every call"_test = [] {
        Log log;
        SceneRegistry registry;
        registry.add("menu", named("Menu", log));
        auto a = registry.create("menu");
        auto b = registry.create("menu");
        expect(a != nullptr);
        expect(b != nullptr);
        expect(a.get() != b.get());
        expect(a->name() == std::string("Menu"));
    };

    "create does not enter the scene"_test = [] {
        Log log;
        SceneRegistry registry;
        registry.add("menu", named("Menu", log));
        auto scene = registry.create("menu");
        expect(log.empty());
    };

    "create returns nullptr for an unknown id"_test = [] {
        SceneRegistry registry;
        expect(registry.create("nope") == nullptr);
        expect(registry.create("") == nullptr);
    };

    "create returns nullptr when the factory returns nothing"_test = [] {
        SceneRegistry registry;
        registry.add("broken", [] { return std::unique_ptr<Scene>(); });
        expect(registry.create("broken") == nullptr);
        expect(registry.has("broken"));
    };
}
