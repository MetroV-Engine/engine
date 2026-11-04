#pragma once

#include "panels/Panel/Panel.hpp"
#include "ui/Button/Button.hpp"
#include "ui/DialogManager/DialogManager.hpp"
#include "ui/NotificationManager/NotificationManager.hpp"

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
