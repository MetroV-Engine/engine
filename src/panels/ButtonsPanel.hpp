#pragma once

#include "Panel.hpp"
#include "components/Button.hpp"
#include "feedback/DialogManager.hpp"
#include "feedback/NotificationManager.hpp"

#include <functional>

// Demo panel showing the Button variants, notifications and dialogs.
class ButtonsPanel : public Panel
{
public:
    ButtonsPanel(NotificationManager &notifications, DialogManager &dialogs, std::function<void()> onQuit);

protected:
    void drawContent() override;

private:
    Button _helloButton;
    Button _cancelButton;
    Button _quitButton;
    Button _warningButton;
    Button _errorButton;
    Button _alertButton;
};
