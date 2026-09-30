#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Registry.hpp"

namespace ECS {
    class ISystem {
        public:
            virtual ~ISystem() = default;

            /** @brief Called once by SystemManager::addSystem, right after construction. */
            virtual void init(ECS::Registry&) {}

            /** @brief Called once by SystemManager::removeSystem, right before destruction. */
            virtual void shutdown(ECS::Registry&) {}

            /** @brief Called once per frame by SystemManager::update, under its own deferred-mutation scope. */
            virtual void update(ECS::Registry &reg, double dt = 0.0) = 0;

            virtual const std::string name() const = 0;
    };

    class SystemManager {
        public:
            SystemManager() = default;
            SystemManager(const SystemManager&) = delete;
            SystemManager& operator=(const SystemManager&) = delete;

            /**
             * @brief Constructs a system in place, takes ownership of it, and
             *        calls its init(reg) hook.
             * @tparam System Concrete system type, must derive from ISystem.
             * @tparam Params Constructor argument types for System.
             * @param reg Registry passed to the new system's init hook.
             * @return Reference to the newly created system.
             */
            template<typename System, typename... Params>
            System& addSystem(Registry& reg, Params&&... params) {
                static_assert(std::is_base_of_v<ISystem, System>,
                    "ECS::SystemManager::addSystem: System must derive from ISystem");
                auto system = std::make_unique<System>(std::forward<Params>(params)...);
                System& ref = *system;
                ref.init(reg);
                _systems.push_back(std::move(system));
                return ref;
            }

            /**
             * @brief Runs every registered system in registration order.
             * @param reg Registry passed to each system's update.
             * @param dt Elapsed time since the previous update, in seconds.
             *
             * Each system runs under its own deferred-mutation scope (see
             * Registry::runDeferred) and is flushed before the next system
             * starts, so structural changes (spawnEntity, addComponent, ...)
             * made by one system are visible to the next system in the same
             * frame.
             */
            void update(Registry& reg, double dt = 0.0) {
                for (auto& system : _systems) {
                    reg.runDeferred([&] {
                        system->update(reg, dt);
                    });
                }
            }

            /**
             * @brief Finds a registered system by its concrete type.
             * @tparam System Concrete system type to look for.
             * @return Pointer to the system, or nullptr when none is registered.
             */
            template<typename System>
            [[nodiscard]] System* getSystem() noexcept {
                for (auto& system : _systems) {
                    if (auto* typed = dynamic_cast<System*>(system.get())) {
                        return typed;
                    }
                }
                return nullptr;
            }

            /**
             * @brief Removes a registered system, calling shutdown(reg) first.
             * @tparam System Concrete system type to remove.
             * @param reg Registry passed to the removed system's shutdown hook.
             * @return True when a matching system was found and removed.
             */
            template<typename System>
            bool removeSystem(Registry& reg) {
                const auto it = std::find_if(_systems.begin(), _systems.end(),
                    [](const std::unique_ptr<ISystem>& system) {
                        return dynamic_cast<System*>(system.get()) != nullptr;
                    });
                if (it == _systems.end()) {
                    return false;
                }
                (*it)->shutdown(reg);
                _systems.erase(it);
                return true;
            }

            /** @brief Returns the number of registered systems. */
            [[nodiscard]] std::size_t size() const noexcept {
                return _systems.size();
            }

        private:
            std::vector<std::unique_ptr<ISystem>> _systems;
    };
}