#include <boost/ut.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include "Engine/Engine.hpp"
#include "Engine/Game/GameCatalog.hpp"

using namespace boost::ut;

// The catalog is process-wide: every test uses names of its own, prefixed
// with "gctest", so the tests do not depend on each other.

namespace gctest {
    // What the install functions did, read by the tests.
    struct Log {
        static inline std::vector<std::string> installs;
    };

    struct Marker {
        int value = 0;
    };

    // Registered by E_GAME below: adds a service, like a real game.
    inline void install(Engine& engine) {
        Log::installs.push_back("gctest_macro");
        engine.addService<Marker>(Marker{42});
    }
}

E_GAME(gctest_macro, gctest::install)

namespace {
    GameCatalog& catalog() {
        return GameCatalog::instance();
    }

    // An install function that records its name.
    GameCatalog::Install recording(const std::string& name) {
        return [name](Engine&) { gctest::Log::installs.push_back(name); };
    }

    bool contains(const std::vector<std::string>& names, const std::string& name) {
        return std::find(names.begin(), names.end(), name) != names.end();
    }
}

void run_game_catalog_tests() {
    // ---- instance

    "instance always returns the same catalog"_test = [] {
        expect(&GameCatalog::instance() == &GameCatalog::instance());
    };

    // ---- add / has

    "add registers a game under its name"_test = [] {
        const std::size_t before = catalog().size();
        expect(catalog().add("gctestAdd", recording("gctestAdd")));
        expect(catalog().has("gctestAdd"));
        expect(catalog().size() == before + 1);
    };

    "add refuses an empty name or an empty function"_test = [] {
        const std::size_t before = catalog().size();
        expect(!catalog().add("", recording("x")));
        expect(!catalog().add("gctestEmptyFunction", GameCatalog::Install{}));
        expect(!catalog().has("gctestEmptyFunction"));
        expect(catalog().size() == before);
    };

    "add refuses a name already taken and keeps the first function"_test = [] {
        expect(catalog().add("gctestTaken", recording("first")));
        expect(!catalog().add("gctestTaken", recording("second")));
        gctest::Log::installs.clear();
        Engine engine;
        catalog().installAll(engine);
        expect(contains(gctest::Log::installs, "first"));
        expect(!contains(gctest::Log::installs, "second"));
    };

    "has is false for an unknown or empty name"_test = [] {
        expect(!catalog().has("gctestNeverRegistered"));
        expect(!catalog().has(""));
    };

    // ---- names

    "names lists every game, sorted"_test = [] {
        expect(catalog().add("gctestNamesZ", recording("Z")));
        expect(catalog().add("gctestNamesA", recording("A")));
        const std::vector<std::string> names = catalog().names();
        expect(names.size() == catalog().size());
        expect(std::is_sorted(names.begin(), names.end()));
        expect(contains(names, "gctestNamesA"));
        expect(contains(names, "gctestNamesZ"));
        expect(!contains(names, ""));
    };

    // ---- installAll

    "installAll calls every game once, sorted by name, and returns their number"_test = [] {
        expect(catalog().add("gctestOrderB", recording("gctestOrderB")));
        expect(catalog().add("gctestOrderA", recording("gctestOrderA")));
        gctest::Log::installs.clear();
        Engine engine;

        const std::size_t installed = catalog().installAll(engine);

        expect(installed == catalog().size());
        expect(gctest::Log::installs.size() == catalog().size());
        const auto a = std::find(gctest::Log::installs.begin(), gctest::Log::installs.end(), "gctestOrderA");
        const auto b = std::find(gctest::Log::installs.begin(), gctest::Log::installs.end(), "gctestOrderB");
        expect(a != gctest::Log::installs.end());
        expect(b != gctest::Log::installs.end());
        expect(a < b);
    };

    "installAll gives the engine the services a game adds"_test = [] {
        Engine engine;
        catalog().installAll(engine);
        expect(engine.services().has<gctest::Marker>());
        expect(engine.services().get<gctest::Marker>().value == 42);
    };

    "installAll lets an exception of an install function through"_test = [] {
        expect(catalog().add("gctestThrows", [](Engine&) { throw std::runtime_error("cannot install"); }));
        Engine engine;
        expect(throws<std::runtime_error>([&] { catalog().installAll(engine); }));
    };

    // ---- E_GAME

    "E_GAME registers the game before main, under the name given to the macro"_test = [] {
        expect(catalog().has("gctest_macro"));
        expect(!catalog().has("gctest::install"));
    };

    "E_GAME records no error for a game registered once"_test = [] {
        const auto& errors = catalog().errors();
        expect(std::none_of(errors.begin(), errors.end(),
                            [](const std::string& error) { return error.rfind("gctest_macro:", 0) == 0; }));
    };

    // ---- registerGame (what E_GAME calls) and errors

    "registerGame returns true and registers a new name"_test = [] {
        expect(game_detail::registerGame("gctestRegister", recording("gctestRegister")));
        expect(catalog().has("gctestRegister"));
    };

    "registerGame records an error, without throwing, for a name already taken"_test = [] {
        expect(game_detail::registerGame("gctestRegisterTwice", recording("first")));
        const std::size_t errors = catalog().errors().size();
        expect(!game_detail::registerGame("gctestRegisterTwice", recording("second")));
        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back().rfind("gctestRegisterTwice: ", 0) == 0_ul);
    };

    "registerGame records an error for an empty name or an empty function"_test = [] {
        const std::size_t errors = catalog().errors().size();
        expect(!game_detail::registerGame("", recording("x")));
        expect(catalog().errors().back().find("non-empty name") != std::string::npos);
        expect(!game_detail::registerGame("gctestNoFunction", GameCatalog::Install{}));
        expect(catalog().errors().size() == errors + 2);
        expect(!catalog().has("gctestNoFunction"));
    };

    "recordError appends '<game>: <what>' to the errors, in order"_test = [] {
        const std::size_t errors = catalog().errors().size();
        catalog().recordError("gctestFirst", "first problem");
        catalog().recordError("gctestSecond", "second problem");
        expect(catalog().errors().size() == errors + 2);
        expect(catalog().errors()[errors] == std::string("gctestFirst: first problem"));
        expect(catalog().errors()[errors + 1] == std::string("gctestSecond: second problem"));
    };
}
