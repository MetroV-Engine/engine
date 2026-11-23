#include <boost/ut.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "ECS/Core/System/SystemCatalog.hpp"

using namespace boost::ut;

// The catalog is process-wide: every test uses names of its own, prefixed
// with "CatTest", so the tests do not depend on each other.
//
// create() refuses a system whose name() differs from its registered name,
// so the test systems take their name as a template argument (default
// constructor, for add<T>) or as a constructor argument (factories).

namespace cattest {
    // A string usable as a template argument.
    template<std::size_t N>
    struct FixedName {
        char value[N];
        constexpr FixedName(const char (&text)[N]) { std::copy_n(text, N, value); }
    };

    // Counts what happens to its instances, to check creation and init.
    struct Counters {
        static inline int constructed = 0;
        static inline int initialized = 0;
        static inline int updated = 0;
    };

    // A system with a fixed name and a default constructor.
    template<FixedName Name>
    class Named : public ECS::ISystem {
        public:
            Named() { ++Counters::constructed; }
            void init(ECS::Registry&) override { ++Counters::initialized; }
            void update(ECS::Registry&, double) override { ++Counters::updated; }
            const std::string name() const override { return Name.value; }
    };

    // A system whose name is chosen at construction (used by factories).
    class Dynamic : public ECS::ISystem {
        public:
            static inline std::vector<std::string>* log = nullptr;   // records the run order

            explicit Dynamic(std::string name) : _name(std::move(name)) { ++Counters::constructed; }
            void init(ECS::Registry&) override { ++Counters::initialized; }
            void update(ECS::Registry&, double) override {
                ++Counters::updated;
                if (log) {
                    log->push_back(_name);
                }
            }
            const std::string name() const override { return _name; }

        private:
            std::string _name;
    };

    // Registered by E_SYSTEM below, with a matching name().
    class MacroSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "cattest::MacroSystem"; }
    };

    // Registered by E_SYSTEM below, with a name() that differs from the macro text.
    class WrongNameSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "WrongNameSystem"; }
    };

    // A template system with two parameters, registered by E_SYSTEM below.
    template<typename A, typename B>
    class TemplateSystem : public ECS::ISystem {
        public:
            void update(ECS::Registry&, double) override {}
            const std::string name() const override { return "cattest::TemplateSystem<int, float>"; }
    };
}

E_SYSTEM(cattest::MacroSystem)
E_SYSTEM(cattest::WrongNameSystem)
E_SYSTEM(cattest::TemplateSystem<int, float>)

namespace {
    ECS::SystemCatalog& catalog() {
        return ECS::SystemCatalog::instance();
    }

    // A factory creating a Dynamic system named `name`.
    ECS::SystemCatalog::Factory factoryNamed(std::string name) {
        return [name] { return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::Dynamic>(name)); };
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
        expect(catalog().add("CatTestAdd", factoryNamed("CatTestAdd")));
        expect(catalog().has("CatTestAdd"));
        expect(catalog().size() == before + 1);
    };

    "add refuses an empty name"_test = [] {
        const std::size_t before = catalog().size();
        expect(!catalog().add("", factoryNamed("")));
        expect(catalog().size() == before);
    };

    "add refuses an empty factory"_test = [] {
        const std::size_t before = catalog().size();
        expect(!catalog().add("CatTestEmptyFactory", ECS::SystemCatalog::Factory{}));
        expect(!catalog().has("CatTestEmptyFactory"));
        expect(catalog().size() == before);
    };

    "add refuses a name already taken and keeps the first factory"_test = [] {
        const int constructed = cattest::Counters::constructed;
        expect(catalog().add("CatTestTaken", factoryNamed("CatTestTaken")));
        expect(!catalog().add("CatTestTaken", [] {
            return std::unique_ptr<ECS::ISystem>(std::make_unique<cattest::Named<"CatTestTaken">>());
        }));
        expect(dynamic_cast<cattest::Dynamic*>(catalog().create("CatTestTaken").get()) != nullptr);
        expect(cattest::Counters::constructed == constructed + 1);
    };

    // ---- add<System>(name)

    "add<System> registers a system created with its default constructor"_test = [] {
        expect(catalog().add<cattest::Named<"CatTestTyped">>("CatTestTyped"));
        const auto system = catalog().create("CatTestTyped");
        expect(dynamic_cast<cattest::Named<"CatTestTyped">*>(system.get()) != nullptr);
    };

    "add<System> refuses an empty name or a name already taken"_test = [] {
        expect(!catalog().add<cattest::Named<"CatTestTypedTwice">>(""));
        expect(catalog().add<cattest::Named<"CatTestTypedTwice">>("CatTestTypedTwice"));
        expect(!catalog().add<cattest::Named<"Other">>("CatTestTypedTwice"));
        expect(dynamic_cast<cattest::Named<"CatTestTypedTwice">*>(catalog().create("CatTestTypedTwice").get()) != nullptr);
    };

    "a system name is case sensitive"_test = [] {
        expect(catalog().add<cattest::Named<"CatTestCase">>("CatTestCase"));
        expect(!catalog().has("cattestcase"));
        expect(catalog().add<cattest::Named<"cattestcase">>("cattestcase"));
    };

    // ---- has

    "has is false for an unknown or empty name"_test = [] {
        expect(!catalog().has("CatTestNeverRegistered"));
        expect(!catalog().has(""));
    };

    // ---- create

    "create returns a new system on every call"_test = [] {
        expect(catalog().add("CatTestCreate", factoryNamed("CatTestCreate")));
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

    "create returns nullptr when the factory returns nothing, without an error"_test = [] {
        expect(catalog().add("CatTestNullFactory", [] { return std::unique_ptr<ECS::ISystem>{}; }));
        const std::size_t errors = catalog().errors().size();
        expect(catalog().create("CatTestNullFactory") == nullptr);
        expect(catalog().errors().size() == errors);
    };

    "create lets an exception of the system's constructor through"_test = [] {
        expect(catalog().add("CatTestThrowingFactory", []() -> std::unique_ptr<ECS::ISystem> {
            throw std::runtime_error("cannot build");
        }));
        expect(throws<std::runtime_error>([] { (void)catalog().create("CatTestThrowingFactory"); }));
    };

    // ---- create: the registered name and name() must match

    "create returns a system whose name() matches its registered name"_test = [] {
        expect(catalog().add("CatTestMatch", factoryNamed("CatTestMatch")));
        const std::size_t errors = catalog().errors().size();

        const auto system = catalog().create("CatTestMatch");

        expect(system != nullptr);
        expect(system->name() == std::string("CatTestMatch"));
        expect(catalog().errors().size() == errors);
    };

    "create refuses a system whose name() differs, and records an error"_test = [] {
        expect(catalog().add("CatTestMismatch", factoryNamed("SomethingElse")));
        const std::size_t errors = catalog().errors().size();

        expect(catalog().create("CatTestMismatch") == nullptr);

        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back() == std::string("CatTestMismatch: name() returns 'SomethingElse'"));
    };

    "create refuses a system whose name() is empty"_test = [] {
        expect(catalog().add("CatTestEmptyName", factoryNamed("")));
        const std::size_t errors = catalog().errors().size();

        expect(catalog().create("CatTestEmptyName") == nullptr);

        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back() == std::string("CatTestEmptyName: name() returns ''"));
    };

    "create records the mismatch again on every call"_test = [] {
        expect(catalog().add("CatTestMismatchTwice", factoryNamed("Other")));
        const std::size_t errors = catalog().errors().size();
        (void)catalog().create("CatTestMismatchTwice");
        (void)catalog().create("CatTestMismatchTwice");
        expect(catalog().errors().size() == errors + 2);
    };

    // ---- createInto

    "createInto adds the system to the manager and calls its init"_test = [] {
        expect(catalog().add("CatTestInto", factoryNamed("CatTestInto")));
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
        expect(catalog().add("CatTestOrderA", factoryNamed("CatTestOrderA")));
        expect(catalog().add("CatTestOrderB", factoryNamed("CatTestOrderB")));
        std::vector<std::string> log;
        cattest::Dynamic::log = &log;
        ECS::Registry registry;
        ECS::SystemManager systems;
        systems.addSystem<cattest::Dynamic>(registry, "code");

        catalog().createInto("CatTestOrderB", systems, registry);
        catalog().createInto("CatTestOrderA", systems, registry);
        systems.update(registry);
        cattest::Dynamic::log = nullptr;

        expect(log == std::vector<std::string>{"code", "CatTestOrderB", "CatTestOrderA"});
    };

    "createInto adds nothing for an unknown name or a factory that returns nothing"_test = [] {
        expect(catalog().add("CatTestIntoNull", [] { return std::unique_ptr<ECS::ISystem>{}; }));
        ECS::Registry registry;
        ECS::SystemManager systems;

        expect(catalog().createInto("CatTestIntoUnknown", systems, registry) == nullptr);
        expect(catalog().createInto("CatTestIntoNull", systems, registry) == nullptr);
        expect(systems.size() == std::size_t{0});
    };

    "createInto adds nothing, and never calls init, for a system whose name() differs"_test = [] {
        expect(catalog().add("CatTestIntoMismatch", factoryNamed("NotTheSame")));
        ECS::Registry registry;
        ECS::SystemManager systems;
        const int initialized = cattest::Counters::initialized;
        const std::size_t errors = catalog().errors().size();

        expect(catalog().createInto("CatTestIntoMismatch", systems, registry) == nullptr);

        expect(systems.size() == std::size_t{0});
        expect(cattest::Counters::initialized == initialized);
        expect(catalog().errors().size() == errors + 1);
    };

    "createInto twice with the same name adds two instances (current behaviour)"_test = [] {
        expect(catalog().add("CatTestIntoTwice", factoryNamed("CatTestIntoTwice")));
        ECS::Registry registry;
        ECS::SystemManager systems;

        ECS::ISystem* first = catalog().createInto("CatTestIntoTwice", systems, registry);
        ECS::ISystem* second = catalog().createInto("CatTestIntoTwice", systems, registry);

        expect(first != nullptr);
        expect(second != nullptr);
        expect(first != second);
        expect(systems.size() == std::size_t{2});
    };

    // ---- names / size

    "names lists every registered name, sorted"_test = [] {
        expect(catalog().add<cattest::Named<"CatTestNamesZ">>("CatTestNamesZ"));
        expect(catalog().add<cattest::Named<"CatTestNamesA">>("CatTestNamesA"));

        const std::vector<std::string> names = catalog().names();

        expect(names.size() == catalog().size());
        expect(std::is_sorted(names.begin(), names.end()));
        expect(std::find(names.begin(), names.end(), "CatTestNamesA") != names.end());
        expect(std::find(names.begin(), names.end(), "CatTestNamesZ") != names.end());
    };

    "names does not list a name that was refused"_test = [] {
        expect(!catalog().add("", factoryNamed("")));
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

    "E_SYSTEM registers a system whose name() differs, but create refuses it"_test = [] {
        expect(catalog().has("cattest::WrongNameSystem"));
        const std::size_t errors = catalog().errors().size();

        expect(catalog().create("cattest::WrongNameSystem") == nullptr);

        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back() == std::string("cattest::WrongNameSystem: name() returns 'WrongNameSystem'"));
    };

    "E_SYSTEM accepts a template type with several parameters"_test = [] {
        // #__VA_ARGS__ keeps the text as written: "<int, float>", one space after the comma.
        expect(catalog().has("cattest::TemplateSystem<int, float>"));
        expect(!catalog().has("cattest::TemplateSystem<int,float>"));
        const auto system = catalog().create("cattest::TemplateSystem<int, float>");
        expect(dynamic_cast<cattest::TemplateSystem<int, float>*>(system.get()) != nullptr);
    };

    // ---- registerSystem (what E_SYSTEM calls) and errors

    "registerSystem returns true and registers a new name"_test = [] {
        expect(ECS::detail::registerSystem<cattest::Named<"CatTestRegister">>("CatTestRegister"));
        expect(catalog().has("CatTestRegister"));
        expect(!hasError("CatTestRegister"));
    };

    "registerSystem records an error, without throwing, for a name already taken"_test = [] {
        expect(ECS::detail::registerSystem<cattest::Named<"CatTestRegisterTwice">>("CatTestRegisterTwice"));
        const std::size_t errors = catalog().errors().size();

        expect(!ECS::detail::registerSystem<cattest::Named<"Other">>("CatTestRegisterTwice"));

        expect(catalog().errors().size() == errors + 1);
        expect(catalog().errors().back().rfind("CatTestRegisterTwice: ", 0) == 0_ul);
        expect(dynamic_cast<cattest::Named<"CatTestRegisterTwice">*>(catalog().create("CatTestRegisterTwice").get()) != nullptr);
    };

    "registerSystem records an error for an empty name"_test = [] {
        const std::size_t errors = catalog().errors().size();
        expect(!ECS::detail::registerSystem<cattest::Named<"x">>(""));
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
