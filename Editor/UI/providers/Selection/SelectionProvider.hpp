/*
    Filename: SelectionProvider.hpp
    Description: Singleton that holds the currently-selected entity. Decouples
                 SceneHierarchy (writes) from Inspector / Viewport (read).

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚████╔╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_PROVIDERS_SELECTIONPROVIDER_HPP
#define PROTO_UI_PROVIDERS_SELECTIONPROVIDER_HPP

#include <optional>
#include "Entity.hpp"

namespace Editor::UI::Providers {

    class SelectionProvider {
    public:
        static SelectionProvider &getInstance()
        {
            static SelectionProvider instance;
            return instance;
        }

        SelectionProvider(const SelectionProvider &) = delete;
        SelectionProvider &operator=(const SelectionProvider &) = delete;

        void selectEntity(const ECS::Entity::entity &e) { _selectedEntity = e; }
        void clearSelection()                            { _selectedEntity.reset(); }
        [[nodiscard]] bool hasSelection() const          { return _selectedEntity.has_value(); }
        [[nodiscard]] const ECS::Entity::entity &getSelectedEntity() const { return _selectedEntity.value(); }

    private:
        SelectionProvider() = default;
        ~SelectionProvider() = default;

        std::optional<ECS::Entity::entity> _selectedEntity;
    };

    inline SelectionProvider &useSelection() { return SelectionProvider::getInstance(); }

}

#endif
