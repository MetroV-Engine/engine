#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>

#include "ECS/ECS.hpp"

/**
 * @brief Non-owning handle to a service, as stored in a scene's registry.
 *
 * The Engine owns the services (see Services); every scene only gets a
 * ServiceRef<T> resource pointing to them, so a scene can come and go without
 * destroying a service the next scene still needs.
 */
template<typename T>
class ServiceRef {
    public:
        explicit ServiceRef(T* service) : _service(service) {}

        T& get() const { return *_service; }
        T& operator*() const { return *_service; }
        T* operator->() const { return _service; }

    private:
        T* _service;
};

/**
 * @brief Reads a service from a scene's registry (from a system, for example).
 * @throws std::out_of_range when T has not been injected into this registry.
 */
template<typename T>
T& getService(ECS::Registry& registry) {
    return registry.getResource<ServiceRef<T>>().get();
}

/** @brief Checks whether a service of type T is available in this registry. */
template<typename T>
bool hasService(const ECS::Registry& registry) noexcept {
    return registry.hasResource<ServiceRef<T>>();
}

/**
 * @brief The services of an Engine: one owned instance per type (asset cache,
 *        audio, renderer, input, game session...).
 *
 * Services outlive scenes. injectInto() gives a registry a ServiceRef<T> for
 * each of them. A service replaced by a second set<T>() is destroyed:
 * references to the old one dangle.
 */
class Services {
    public:
        Services() = default;
        Services(const Services&) = delete;
        Services& operator=(const Services&) = delete;

        /**
         * @brief Constructs or replaces the service of type T.
         * @param params Constructor arguments forwarded to T.
         * @return Reference to the stored service.
         */
        template<typename T, typename... Params>
        T& set(Params&&... params) {
            T& service = _store.set<T>(std::forward<Params>(params)...);
            _injectors[ECS::resourceId<T>()] = [this](ECS::Registry& registry) {
                registry.setResource<ServiceRef<T>>(&_store.get<T>());
            };
            return service;
        }

        /** @throws std::out_of_range when T has never been set. */
        template<typename T>
        [[nodiscard]] T& get() { return _store.get<T>(); }

        /** @copydoc get() */
        template<typename T>
        [[nodiscard]] const T& get() const { return _store.get<T>(); }

        template<typename T>
        [[nodiscard]] bool has() const noexcept { return _store.has<T>(); }

        /** @brief Number of services held. */
        [[nodiscard]] std::size_t size() const noexcept { return _injectors.size(); }

        /** @brief Gives the registry a ServiceRef<T> for every service. */
        void injectInto(ECS::Registry& registry) const {
            for (const auto& entry : _injectors)
                entry.second(registry);
        }

        /** @brief Destroys every service. */
        void clear() {
            _injectors.clear();
            _store = ECS::ResourceStore{};
        }

    private:
        ECS::ResourceStore _store;
        std::unordered_map<ECS::ResourceId, std::function<void(ECS::Registry&)>> _injectors;
};
