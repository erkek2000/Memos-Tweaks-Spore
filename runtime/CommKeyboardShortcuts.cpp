#include "stdafx.h"
#include "CommKeyboardShortcuts.h"

#include <Spore\Simulator\SubSystem\CommManager.h>
#include <Spore\UTFWin\IWinProc.h>
#include <Spore\UTFWin\IWindowManager.h>

namespace
{
    class CommKeyboardProc final : public UTFWin::DefaultWinProc<UTFWin::kEventFlagBasicInput>
    {
    public:
        int GetPriority() const override
        {
            // Run before ordinary UI procedures so TAB does not also change focus.
            return 1000;
        }

        bool HandleUIMessage(UTFWin::IWindow*, const UTFWin::Message& message) override
        {
            if (!message.IsType(UTFWin::kMsgKeyDown) &&
                !message.IsType(UTFWin::kMsgKeyDown2))
            {
                return false;
            }

            const bool closeKey =
                (ERKEK_CLOSE_WITH_ESCAPE && message.Key.vkey == VK_ESCAPE) ||
                (ERKEK_CLOSE_WITH_TAB && message.Key.vkey == VK_TAB);
            if (!closeKey)
            {
                return false;
            }

            Simulator::cCommManager* manager = Simulator::cCommManager::Get();
            if (manager == nullptr || !manager->IsCommScreenActive())
            {
                return false;
            }

            UTFWin::IButton* exitButton = manager->GetCommButton(Simulator::kBtnExit);
            UTFWin::IWindow* exitWindow = exitButton == nullptr ? nullptr : exitButton->ToWindow();
            if (exitWindow == nullptr)
            {
                return false;
            }

            const UTFWin::WindowFlags flags = exitWindow->GetFlags();
            if ((flags & UTFWin::kWinFlagVisible) == 0 ||
                (flags & UTFWin::kWinFlagEnabled) == 0)
            {
                // Submenus can hide or disable Goodbye. Do not bypass their own
                // cancellation/confirmation flow.
                return false;
            }

            UTFWin::Message click{};
            click.source = exitWindow;
            click.eventType = UTFWin::kMsgButtonClick;
            WindowManager.SendMsg(exitWindow, exitWindow, click, true);
            return true;
        }
    };

    IWinProcPtr sCommKeyboardProc;
    IWindowPtr sMainWindow;
}

void ERKEK2000QoL::InstallCommKeyboardShortcuts()
{
    if (sCommKeyboardProc != nullptr)
    {
        return;
    }

    UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
    if (mainWindow == nullptr)
    {
        SporeDebugPrint("ERKEK2000 QoL Runtime: main UI unavailable; dialogue shortcuts not installed.");
        return;
    }

    sMainWindow = mainWindow;
    sCommKeyboardProc = new CommKeyboardProc();
    mainWindow->AddWinProc(sCommKeyboardProc.get());
}

void ERKEK2000QoL::RemoveCommKeyboardShortcuts()
{
    if (sMainWindow != nullptr && sCommKeyboardProc != nullptr)
    {
        sMainWindow->RemoveWinProc(sCommKeyboardProc.get());
    }
    sCommKeyboardProc = nullptr;
    sMainWindow = nullptr;
}
