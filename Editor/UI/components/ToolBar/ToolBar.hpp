/*
    Filename: ToolBar.hpp
    Description: Top toolbar panel. Currently hosts the PlayButton; designed
                 to grow with additional editor actions (save, build, etc.)

    ███╗   ███╗███████╗████████╗██████╗  ██████╗ ██╗   ██╗
    ████╗ ████║██╔════╝╚══██╔══╝██╔══██╗██╔═══██╗██║   ██║
    ██╔████╔██║█████╗     ██║   ██████╔╝██║   ██║██║   ██║
    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██╗██║   ██║╚██╗ ██╔╝
    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝ ╚════╝
    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝   ╚═══╝
*/

#ifndef PROTO_UI_TOOLBAR_HPP
#define PROTO_UI_TOOLBAR_HPP

#include "../common/Panel/Panel.hpp"
#include "PlayButton/PlayButton.hpp"

namespace Editor::UI::Pages { class EditorPage; }

namespace Editor::UI::Components {

    class ToolBar : public Panel {
    public:
        explicit ToolBar(Pages::EditorPage *page);

        void update(ECS::registry &reg) override;
        void render(std::vector<ECS::System::ISystem *> systems) override;

        [[nodiscard]] int getExtraFlags() const override;
        [[nodiscard]] const char *getName() const override { return "Toolbar"; }

        PlayButton &playButton() { return _playButton; }

    private:
        Pages::EditorPage *_page;
        ECS::registry *_reg = nullptr;
        PlayButton _playButton;
    };

}

#endif
