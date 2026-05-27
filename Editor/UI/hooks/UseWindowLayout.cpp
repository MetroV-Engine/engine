#include "UseWindowLayout.hpp"

#include <algorithm>

#include "../providers/Scale/ScaleProvider.hpp"
#include "../providers/Theme/ThemeProvider.hpp"

namespace Editor::UI::Hooks {

    WindowLayout useWindowLayout()
    {
        const auto win = Providers::useScale().getWinSize();
        const auto &d  = Providers::useDesign();

        WindowLayout L;
        L.winWidth   = static_cast<float>(win.x);
        L.winHeight  = static_cast<float>(win.y);

        L.toolbarHeight  = std::max(d.minToolbarHeight, L.winHeight * d.toolbarHeightRatio);
        L.projectWidth   = std::max(d.minProjectWidth,
                                    std::min(d.maxSidePanelWidth, L.winWidth * d.sidePanelRatio));
        L.inspectorWidth = std::max(d.minInspectorWidth,
                                    std::min(d.maxSidePanelWidth, L.winWidth * d.sidePanelRatio));
        L.contentHeight  = L.winHeight - L.toolbarHeight;
        L.viewportWidth  = L.winWidth  - L.projectWidth;

        return L;
    }

}
