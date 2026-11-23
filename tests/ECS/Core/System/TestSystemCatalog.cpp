#include <boost/ut.hpp>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ECS/Core/System/SystemCatalog.hpp"

using namespace boost::ut;

// The catalog is process-wide: every test uses names of its own, prefixed
// with "CatTest", so the tests do not depend on each other.

namespace cattest {
    // Counts what happens to its instances, to check creation and init.
    struct Counters {
        static inline int constructed = 0;
        static inline int initialized = 0;
        static inline int updated = 0;
    };

    class CountingSystem : public ECS::ISystem {
        public:
            CountingSystem() { ++Counters::constructed; }
            void init(ECS::Registry&) override { ++Counters::initialized; }
            void update(ECS::Registry&, double) override { ++Counters::updated; }
            const std::string name() const override { return "cattest::CountingSystem"; }
    };

    class OtherSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "cattest::OtherSystem"; }
    };

    // Registered before main by E_SYSTEM, below.
    class MacroSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "cattest::MacroSystem"; }
    };

    // Records the order systems run in.
    class OrderSystem : public ECS::ISystem {
        public:
            static inline std::vector<std::string>* log = nullptr;
            explicit OrderSystem(std::string id = "?") : _id(std::move(id)) {}
            void update(ECS::Registry&, double) override {
                if (log) {
                    log->push_back(_id);
                }
            }
            const std::string name() const override { return "cattest::OrderSystem"; }

        private:
            std::string _id;
    };
}

E_SYSTEM(cattest::MacroSystem)

namespace {
    ECS::SystemCatalog& catalog() {
        return ECS::SystemCatalog::instance();
    }

    ECS::SystemCatalog::Factory countingFactory() {
        return [] { return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::CountingSystem>()); };
    }

    bool hasError(const std::string& part) {
        const auto& errors = catalog().errors();
        return std::any_of(errors.begin(), errors.end(),
                           [&](const std::string& error) { return error.find(part) != std::string::npos; });
    }
}

void run_system_catalog_tests() {
    // ---- instance

    "instance always returns the same catalog"_test = [] {
        expect(&ECS::SystemCatalog::instance() == &ECS::SystemCatalog::instance());
    };

    // ---- add(name, factory)

    "add registers a factory under a name"_test = [] {
        const std::size_t before = catalog().size();
        expect(catalog().add("CatTestAdd", countingFactory()));
        expect(catalog().has("CatTestAdd"));
        expect(catalog().size() == before + 1);
    };

    "add refuses an empty name"_test = [] {
        const std::size_t before = catalog().size();
        expect(!catalog().add("", countingFactory()));
        expect(catalog().size() == before);
    };

    "add refuses an empty factory"_test = [] {
        const std::size_t before = catalog().size();
        expect(!catalog().add("CatTestEmptyFactory", ECS::SystemCatalog::Factory{}));
        expect(!catalog().has("CatTestEmptyFactory"));
        expect(catalog().size() == before);
    };

    "add refuses a name already taken and keeps the first factory"_test = [] {
        expect(catalog().add("CatTestTaken", countingFactory()));
        expect(!catalog().add("CatTestTaken", [] {
            return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::OtherSystem>());
        }));
        expect(dynamic_cast<cattest::CountingSystem*>(catalog().create("CatTestTaken").get()) != nullptr);
    };

    // ---- add<System>(name)

    "add<System> registers a system created with its default constructor"_test = [] {
        expect(catalog().add<cattest::OtherSystem>("CatTestTyped"));
        const auto system = catalog().create("CatTestTyped");
        expect(dynamic_cast<cattest::OtherSystem*>(system.get()) != nullptr);
    };

    "add<System> refuses an empty name or a name already taken"_test = [] {
        expect(!catalog().add<cattest::OtherSystem>(""));
        expect(catalog().add<cattest::OtherSystem>("CatTestTypedTwice"));
        expect(!catalog().add<cattest::CountingSystem>("CatTestTypedTwice"));
        expect(dynamic_cast<cattest::OtherSystem*>(catalog().create("CatTestTypedTwice").get()) != nullptr);
    };

    "a system name is case sensitive"_test = [] {
        expect(catalog().add<cattest::OtherSystem>("CatTestCase"));
        expect(!catalog().has("cattestcase"));
        expect(catalog().add<cattest::OtherSystem>("cattestcase"));
    };

    // ---- has

    "has is false for an unknown or empty name"_test = [] {
        expect(!catalog().has("CatTestNeverRegistered"));
        expect(!catalog().has(""));
    };

    // ---- create

    "create returns a new system on every call"_test = [] {
        expect(catalog().add("CatTestCreate", countingFactory()));
        const int before = cattest::Counters::constructed;

        const auto first = catalog().create("CatTestCreate");
        const auto second = catalog().create("CatTestCreate");

        expect(first != nullptr);
        expect(second != nullptr);
        expect(first.get() != second.get());
        expect(cattest::Counters::constructed == before + 2);
    };

    "create returns nullptr for an unknown name"_test = [] {
        expect(catalog().create("CatTestUnknown") == nullptr);
        expect(catalog().create("") == nullptr);
    };

    "create returns nullptr when the factory returns nothing"_test = [] {
        expect(catalog().add("CatTestNullFactory", [] { return std::unique_ptr<ECS::ISystem>{}; }));
        expect(catalog().create("CatTestNullFactory") == nullptr);
    };

    "create lets an exception of the system's constructor through"_test = [] {
        expect(catalog().add("CatTestThrowingFactory", []() -> std::unique_ptr<ECS::ISystem> {
            throw std::runtime_error("cannot build");
        }));
        expect(throws<std::runtime_error>([] { (void)catalog().create("CatTestThrowingFactory"); }));
    };

    // ---- createInto

    "createInto adds the system to the manager and calls its init"_test = [] {
        expect(catalog().add("CatTestInto", countingFactory()));
        ECS::Registry registry;
        ECS::SystemManager systems;
        const int initialized = cattest::Counters::initialized;
        const int updated = cattest::Counters::updated;

        ECS::ISystem* added = catalog().createInto("CatTestInto", systems, registry);

        expect(added != nullptr);
        expect(systems.size() == std::size_t{1});
        expect(cattest::Counters::initialized == initialized + 1);
        systems.update(registry);
        expect(cattest::Counters::updated == updated + 1);
    };

    "createInto adds the systems after the ones already there, in call order"_test = [] {
        expect(catalog().add("CatTestOrderA", [] { return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::OrderSystem>("A")); }));
        expect(catalog().add("CatTestOrderB", [] { return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::OrderSystem>("B")); }));
        std::vector<std::string> log;
        cattest::OrderSystem::log = &log;
        ECS::Registry registry;
        ECS::SystemManager systems;
        systems.addSystem<cattest::OrderSystem>(registry, "code");

        catalog().createInto("CatTestOrderB", systems, registry);
        catalog().createInto("CatTestOrderA", systems, registry);
        systems.update(registry);
        cattest::OrderSystem::log = nullptr;

        expect(log == std::vector<std::string>{"code", "B", "A"});
    };

    "createInto adds nothing for an unknown name or a factory that returns nothing"_test = [] {
        expect(catalog().add("CatTestIntoNull", [] { return std::unique_ptr<ECS::ISystem>{}; }));
        ECS::Registry registry;
        ECS::SystemManager systems;

        expect(catalog().createInto("CatTestIntoUnknown", systems, registry) == nullptr);
        expect(catalog().createInto("CatTestIntoNull", systems, registry) == nullptr);
        expect(systems.size() == std::size_t{0});
    };

    // ---- names / size

    "names lists every registered name, sorted"_test = [] {
        expect(catalog().add<cattest::OtherSystem>("CatTestNamesZ"));
        expect(catalog().add<cattest::OtherSystem>("CatTestNamesA"));

        const std::vector<std::string> names = catalog().names();

        expect(names.size() == catalog().size());
        expect(std::is_sorted(names.begin(), names.end()));
        expect(std::find(names.begin(), names.end(), "CatTestNamesA") != names.end());
        expect(std::find(names.begin(), names.end(), "CatTestNamesZ") != names.end());
    };

    "names does not list a name that was refused"_test = [] {
        expect(!catalog().add("", countingFactory()));
        expect(!catalog().add("CatTestRefusedName", ECS::SystemCatalog::Factory{}));
        const std::vector<std::string> names = catalog().names();
        expect(std::find(names.begin(), names.end(), "") == names.end());
        expect(std::find(names.begin(), names.end(), "CatTestRefusedName") == names.end());
    };

    // ---- E_SYSTEM

    "E_SYSTEM registers the system before main, under the name written in the macro"_test = [] {
        expect(catalog().has("cattest::MacroSystem"));
        expect(!catalog().has("MacroSystem"));   // the name is the macro argument as written
        const auto system = catalog().create("cattest::MacroSystem");
        expect(dynamic_cast<cattest::MacroSystem*>(system.get()) != nullptr);
    };

    "E_SYSTEM records no error for a system registered once"_test = [] {
        expect(!hasError("cattest::MacroSystem"));
    };

    // ---- registerSystem (what E_SYSTEM calls) and errors

    "registerSystem returns true and registers a new name"_test = [] {
        expect(ECS::detail::registerSystem<cattest::OtherSystem>("CatTestRegister"));
        expect(catalog().has("CatTestRegister"));
        expect(!hasError("CatTestRegister"));
    };

    "registerSystem records an error, without throwing, for a name already taken"_test = [] {
        expect(ECS::detail::registerSystem<cattest::OtherSystem>("CatTestRegisterTwice"));
        const std::size_t errors = catalog().errors().size();

        expect(!ECS::detail::registerSystem<cattest::CountingSystem>("CatTestRegisterTwice"));

        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back().rfind("CatTestRegisterTwice: ", 0) == 0_ul);
        expect(dynamic_cast<cattest::OtherSystem*>(catalog().create("CatTestRegisterTwice").get()) != nullptr);
    };

    "registerSystem records an error for an empty name"_test = [] {
        const std::size_t errors = catalog().errors().size();
        expect(!ECS::detail::registerSystem<cattest::OtherSystem>(""));
        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back().find("non-empty name") != std::string::npos);
    };

    "recordError appends '<name>: <what>' to the errors, in order"_test = [] {
        const std::size_t errors = catalog().errors().size();
        catalog().recordError("CatTestFirst", "first problem");
        catalog().recordError("CatTestSecond", "second problem");

        expect(catalog().errors().size() == errors + 2);
        expect(catalog().errors()[errors] == std::string("CatTestFirst: first problem"));
        expect(catalog().errors()[errors + 1] == std::string("CatTestSecond: second problem"));
    };
}
