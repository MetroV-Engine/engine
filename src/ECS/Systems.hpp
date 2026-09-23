#pragma once

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

            virtual void update(ECS::Registry &reg, double dt = 0.0) = 0;

            virtual const std::string name() const = 0;
    };

    class SystemManager {
        public:
            SystemManager() = default;
            SystemManager(const SystemManager&) = delete;
            SystemManager& operator=(const SystemManager&) = delete;

            /**
             * @brief Constructs a system in place and takes ownership of it.
             * @tparam System Concrete system type, must derive from ISystem.
             * @tparam Params Constructor argument types for System.
             * @return Reference to the newly created system.
             */
            template<typename System, typename... Params>
            System& addSystem(Params&&... params) {
                static_assert(std::is_base_of_v<ISystem, System>,
                    "ECS::SystemManager::addSystem: System must derive from ISystem");
                auto system = std::make_unique<System>(std::forward<Params>(params)...);
                System& ref = *system;
                _systems.push_back(std::move(system));
                return ref;
            }

            /**
             * @brief Runs every registered system in registration order.
             * @param reg Registry passed to each system's update, under
             *        deferred-mutation mode (see Registry::runDeferred).
             * @param dt Elapsed time since the previous update, in seconds.
             */
            void update(Registry& reg, double dt = 0.0) {
                reg.runDeferred([&] {
                    for (auto& system : _systems) {
                        system->update(reg, dt);
                    }
                });
            }

            /** @brief Returns the number of registered systems. */
            [[nodiscard]] std::size_t size() const noexcept {
                return _systems.size();
            }

        private:
            std::vector<std::unique_ptr<ISystem>> _systems;
    };
}