#pragma once

#include <utility>

namespace ECS {
    /**
     * @brief Type-erased interface shared by all runtime resource holders.
     *
     * The interface exposes nothing beyond destruction: typed access remains
     * in ResourceHolder<T>, so Registry can store holders for arbitrary
     * resource types without knowing them in advance.
     */
    class IResourceHolder {
        public:
            virtual ~IResourceHolder() = default;
    };

    /**
     * @brief Type-erased box around a single resource value.
     * @tparam Resource Resource type held by this box.
     *
     * Resource may be a value type (owned state) or a raw pointer (a
     * non-owning reference to state that lives elsewhere), both stored the
     * same way.
     */
    template<typename Resource>
    class ResourceHolder final : public IResourceHolder {
        public:
            template<typename... Params>
            explicit ResourceHolder(Params&&... params) : _value(std::forward<Params>(params)...) {}

            /** @brief Returns the held resource. */
            [[nodiscard]] Resource& value() noexcept { return _value; }

            /** @copydoc value() */
            [[nodiscard]] const Resource& value() const noexcept { return _value; }

        private:
            Resource _value;
    };
}
