#pragma once

#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class Engine;

/**
 * @brief Process-wide list of the games linked into (or, later, loaded by)
 *        the program, and how each one prepares the Engine.
 *
 * A game is the code a scene file needs: its components (E_REFLECT), its
 * systems (E_SYSTEM), and an install function that gives the Engine what the
 * game needs besides its scene files (services: a terminal, an asset
 * cache...). The game registers that function with E_GAME, before main; a
 * generic program (the runtime) calls installAll() and never names a game.
 *
 * How the game's code gets into the program does not matter: linked into the
 * executable today (one binary), loaded from a shared library by a plugin
 * loader later. Either way its registrations run when its code is loaded.
 *
 * Not thread-safe: registrations happen before main, on one thread.
 */
class GameCatalog {
    public:
        /** @brief Prepares the Engine for the game (adds its services...). */
        using Install = std::function<void(Engine&)>;

        GameCatalog(const GameCatalog&) = delete;
        GameCatalog& operator=(const GameCatalog&) = delete;

        /** @brief Returns the process-wide catalog. */
        [[nodiscard]] static GameCatalog& instance() {
            static GameCatalog catalog;
            return catalog;
        }

        /**
         * @brief Registers a game's install function under its name.
         * @return false (nothing registered) when the name is empty, the
         *         function is empty or the name is already taken.
         */
        bool add(std::string name, Install install) {
            if (name.empty() || !install || has(name)) {
                return false;
            }
            _games.emplace(std::move(name), std::move(install));
            return true;
        }

        /** @brief True when a game is registered under this name. */
        [[nodiscard]] bool has(std::string_view name) const {
            return _games.find(name) != _games.end();
        }

        /** @brief Every registered game, sorted by name. */
        [[nodiscard]] std::vector<std::string> names() const {
            std::vector<std::string> result;
            result.reserve(_games.size());
            for (const auto& [name, install] : _games) {
                result.push_back(name);
            }
            return result;
        }

        /** @brief Number of registered games. */
        [[nodiscard]] std::size_t size() const noexcept {
            return _games.size();
        }

        /**
         * @brief Calls the install function of every game, sorted by name.
         *
         * Exceptions of an install function propagate (the games after it
         * are not installed): a game that cannot prepare the Engine should
         * stop the program at start-up.
         * @return Number of games installed.
         */
        std::size_t installAll(Engine& engine) const {
            std::size_t installed = 0;
            for (const auto& [name, install] : _games) {
                install(engine);
                ++installed;
            }
            return installed;
        }

        /**
         * @brief Failures of the automatic registration (E_GAME), as
         *        "<game>: <what>", in the order they happened. Each one was
         *        also written to std::cerr when it was recorded.
         */
        [[nodiscard]] const std::vector<std::string>& errors() const noexcept {
            return _errors;
        }

        /** @brief Records a failure and reports it on std::cerr, as "[games] <game>: <what>". */
        void recordError(std::string_view game, std::string_view what) {
            std::string message = std::string(game) + ": " + std::string(what);
            // Through std::cerr until the engine has a logger. Including
            // <iostream> makes std::cerr usable during static initialisation.
            std::cerr << "[games] " << message << '\n';
            _errors.push_back(std::move(message));
        }

    private:
        GameCatalog() = default;

        // Ordered: names() and installAll() follow the names. std::less<> allows lookups by string_view.
        std::map<std::string, Install, std::less<>> _games;
        std::vector<std::string> _errors;
};

namespace game_detail {
    /** @brief Specialized by E_GAME for a tag type of its own; holds the registering variable. */
    template<typename Tag>
    struct Registration;

    // Called by every E_GAME before main. Exceptions cannot leave: they would
    // abort the program before main.
    inline bool registerGame(std::string_view name, GameCatalog::Install install) noexcept {
        try {
            if (name.empty()) {
                GameCatalog::instance().recordError(name, "a game needs a non-empty name");
                return false;
            }
            if (!GameCatalog::instance().add(std::string(name), std::move(install))) {
                GameCatalog::instance().recordError(name, "the name is already taken by another game");
                return false;
            }
            return true;
        } catch (const std::exception& error) {
            GameCatalog::instance().recordError(name, error.what());
        } catch (...) {
            GameCatalog::instance().recordError(name, "unknown error");
        }
        return false;
    }
}

/**
 * @brief Registers a game in the GameCatalog, before main.
 *
 * Usage, at global scope, in a file compiled into the game:
 * @code
 * namespace demo { inline void install(Engine& engine) { engine.addService<Terminal>(); } }
 * E_GAME(demo, demo::install)
 * @endcode
 *
 * @param name An identifier: the game's name ("demo").
 * @param install A function `void(Engine&)`.
 *
 * Like E_SYSTEM, it must be compiled into the program: a file of the game
 * that nothing references, in a static library, would be dropped by the
 * linker. Link the game as an OBJECT library, with WHOLE_ARCHIVE, or as a
 * shared library.
 */
#define E_GAME(name, install)                                                   \
    struct EGameTag_##name {};                                                  \
    template<> struct game_detail::Registration<EGameTag_##name> {              \
        static inline const bool registered_ =                                  \
            ::game_detail::registerGame(#name, install);                        \
    };
