#pragma once

#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "ISystem.hpp"
#include "SystemManager.hpp"

namespace ECS {
    /**
     * @brief Process-wide list of the systems that can be created from their name.
     *
     * A scene file lists the systems it runs by name, and the editor lets the
     * user choose among the names listed here: the code defines which systems
     * exist, the data chooses which ones a scene uses.
     *
     * Systems are added by E_SYSTEM (automatically, before main) or by add().
     * The catalog only creates systems; a SystemManager owns and runs them.
     *
     * A system's registered name and its ISystem::name() must be the same
     * string: scene files store the name, and a system whose name() differs
     * could not be found again. create() checks it and refuses the system.
     *
     * Automatic registration relies on static initialisation, like E_REFLECT.
     * A system is only referenced by its name, so nothing in the code points to
     * it: put E_SYSTEM in the system's header and include that header from a
     * .cpp of the executable, or link a static library with WHOLE_ARCHIVE.
     * A shared library is loaded whole, so its systems are always registered.
     *
     * Not thread-safe: registrations happen before main, on one thread.
     */
    class SystemCatalog {
        public:
            /** @brief Creates a new system; may return nullptr. */
            using Factory = std::function<std::unique_ptr<ISystem>()>;

            SystemCatalog(const SystemCatalog&) = delete;
            SystemCatalog& operator=(const SystemCatalog&) = delete;

            /** @brief Returns the process-wide catalog. */
            [[nodiscard]] static SystemCatalog& instance() {
                static SystemCatalog catalog;
                return catalog;
            }

            /**
             * @brief Registers a factory under a name.
             * @return false (nothing registered) when the name is empty, the
             *         factory is empty or the name is already taken.
             */
            bool add(std::string name, Factory factory) {
                if (name.empty() || !factory || has(name)) {
                    return false;
                }
                _factories.emplace(std::move(name), std::move(factory));
                return true;
            }

            /**
             * @brief Registers System under a name, created with its default constructor.
             * @return false (nothing registered) when the name is empty or already taken.
             */
            template<typename System>
            bool add(std::string name) {
                static_assert(std::is_base_of_v<ISystem, System>,
                    "ECS::SystemCatalog::add: System must derive from ECS::ISystem");
                static_assert(std::is_default_constructible_v<System>,
                    "ECS::SystemCatalog::add: a system created from its name needs a default constructor");
                return add(std::move(name), [] { return std::unique_ptr<ISystem>(std::make_unique<System>()); });
            }

            /** @brief True when a system is registered under this name. */
            [[nodiscard]] bool has(std::string_view name) const {
                return _factories.find(name) != _factories.end();
            }

            /**
             * @brief Creates a new system from its name.
             *
             * The created system's name() must equal name. Otherwise it is
             * destroyed (its init was never called), nullptr is returned and
             * "<name>: name() returns '<other>'" is added to errors(), on every
             * such call. The check is done in every build, not only in debug:
             * it costs one string comparison per created system.
             * @return The system, or nullptr when the name is unknown, its
             *         factory returned nothing or its name() differs.
             *         Exceptions of the factory (the system's constructor)
             *         propagate.
             */
            [[nodiscard]] std::unique_ptr<ISystem> create(std::string_view name) const {
                const auto it = _factories.find(name);
                if (it == _factories.end()) {
                    return nullptr;
                }
                std::unique_ptr<ISystem> system = it->second();
                if (system && system->name() != name) {
                    appendError(name, "name() returns '" + system->name() + "'");
                    return nullptr;
                }
                return system;
            }

            /**
             * @brief Creates the system named name and adds it to systems
             *        (which calls its init(reg)), after the systems already there.
             *
             * Called twice with the same name, it adds two instances: a
             * SystemManager may hold several systems of the same name.
             * @return The added system, or nullptr (nothing added) when
             *         create() returns nothing (unknown name, factory that
             *         returned nothing, name() that differs).
             */
            ISystem* createInto(std::string_view name, SystemManager& systems, Registry& reg) const {
                std::unique_ptr<ISystem> system = create(name);
                if (!system) {
                    return nullptr;
                }
                return &systems.addSystem(reg, std::move(system));
            }

            /** @brief Every registered name, sorted (for the editor's list of systems). */
            [[nodiscard]] std::vector<std::string> names() const {
                std::vector<std::string> result;
                result.reserve(_factories.size());
                for (const auto& [name, factory] : _factories) {
                    result.push_back(name);
                }
                return result;
            }

            /** @brief Number of registered systems. */
            [[nodiscard]] std::size_t size() const noexcept {
                return _factories.size();
            }

            /**
             * @brief Failures of the automatic registration (E_SYSTEM) and
             *        systems refused by create() because their name() differs,
             *        as "<SystemName>: <what>", in the order they happened.
             *
             * Each one was also written to std::cerr when it was recorded.
             */
            [[nodiscard]] const std::vector<std::string>& errors() const noexcept {
                return _errors;
            }

            /**
             * @brief Records a failure and reports it right away on std::cerr,
             *        as "[systems] <SystemName>: <what>".
             */
            void recordError(std::string_view systemName, std::string_view what) {
                appendError(systemName, what);
            }

        private:
            SystemCatalog() = default;

            // const: create() records a refused system without being a modifier
            // of the catalog; the error list is diagnostics, not catalog state.
            void appendError(std::string_view systemName, std::string_view what) const {
                std::string message = std::string(systemName) + ": " + std::string(what);
                // Through std::cerr until the engine has a logger. Including
                // <iostream> guarantees std::cerr is usable during static
                // initialisation, where E_SYSTEM registrations run.
                std::cerr << "[systems] " << message << '\n';
                _errors.push_back(std::move(message));
            }

            // Ordered: names() is sorted for free. std::less<> allows lookups by string_view.
            std::map<std::string, Factory, std::less<>> _factories;
            mutable std::vector<std::string> _errors;
    };

    namespace detail {
        /** @brief Specialized by E_SYSTEM(T); holds the variable whose initialisation registers T. */
        template<typename T>
        struct SystemRegistration;

        // Called by every E_SYSTEM before main. Exceptions cannot leave: they
        // would abort the program before main.
        template<typename System>
        bool registerSystem(std::string_view name) noexcept {
            try {
                if (name.empty()) {
                    SystemCatalog::instance().recordError(name, "a system needs a non-empty name");
                    return false;
                }
                if (!SystemCatalog::instance().add<System>(std::string(name))) {
                    SystemCatalog::instance().recordError(name, "the name is already taken by another system");
                    return false;
                }
                return true;
            } catch (const std::exception& error) {
                SystemCatalog::instance().recordError(name, error.what());
            } catch (...) {
                SystemCatalog::instance().recordError(name, "unknown error");
            }
            return false;
        }
    }
}

/**
 * @brief Registers a system in the SystemCatalog under its name, before main.
 *
 * Usage, at global scope, in the system's header, after the class:
 * @code
 * class MovementSystem : public ECS::ISystem { ... };
 * E_SYSTEM(MovementSystem)
 * @endcode
 *
 * The name is the macro argument as written, each run of spaces reduced to
 * one: E_SYSTEM(game::MovementSystem) gives "game::MovementSystem",
 * E_SYSTEM(game :: MovementSystem) gives "game :: MovementSystem", and
 * E_SYSTEM(::game::MovementSystem) gives "::game::MovementSystem". Write it
 * exactly as the system's ISystem::name() returns it, with no leading "::"
 * and no spaces: SystemCatalog::create() refuses a system whose name()
 * differs. Qualify it to avoid collisions between modules.
 *
 * A template type may be given (the macro is variadic, so its commas are
 * fine): E_SYSTEM(game::Spawner<Enemy, 3>) gives "game::Spawner<Enemy, 3>",
 * spaces after the commas included as written.
 *
 * The system must derive from ECS::ISystem and have a default constructor.
 * A name taken twice is recorded in SystemCatalog::errors(), not thrown.
 *
 * In the header and not in a .cpp: nothing in the code references a system
 * chosen by name, so the object file of a .cpp in a static library would be
 * dropped by the linker, and the system never registered.
 */
#define E_SYSTEM(...)                                                         \
    template<> struct ECS::detail::SystemRegistration<__VA_ARGS__> {          \
        static inline const bool registered_ =                                \
            ::ECS::detail::registerSystem<__VA_ARGS__>(#__VA_ARGS__);         \
    };
