#include "stdafx.h"
#include "SpaceHotbarKeyboardShortcuts.h"

#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\UI\SpaceToolPanelUI.h>
#include <Spore\UTFWin\IWinProc.h>
#include <Spore\UTFWin\IWindowManager.h>

namespace
{
    bool IsUsable(UTFWin::IWindow* window)
    {
        for (UTFWin::IWindow* current = window; current != nullptr; current = current->GetParent())
        {
            const UTFWin::WindowFlags flags = current->GetFlags();
            if ((flags & UTFWin::kWinFlagVisible) == 0 ||
                (flags & UTFWin::kWinFlagEnabled) == 0)
            {
                return false;
            }
        }
        return true;
    }

    bool DispatchToVisibleToolPanel(UTFWin::IWindow* window, const UTFWin::Message& message)
    {
        if (window == nullptr || !IsUsable(window))
        {
            return false;
        }

        // SpaceToolPanelUI already owns the game's slot ordering, tool state,
        // cooldown checks and normal selection path. Some panels do not receive
        // root keyboard events, so give the original procedure the key directly.
        for (UTFWin::IWinProc* procedure : window->procedures())
        {
            UI::SpaceToolPanelUI* panel = object_cast<UI::SpaceToolPanelUI>(procedure);
            if (panel != nullptr && panel->HandleUIMessage(window, message))
            {
                return true;
            }
        }

        for (UTFWin::IWindow* child : window->children())
        {
            if (DispatchToVisibleToolPanel(child, message))
            {
                return true;
            }
        }
        return false;
    }

    class SpaceHotbarKeyboardProc final : public UTFWin::DefaultWinProc<UTFWin::kEventFlagBasicInput>
    {
    public:
        int GetPriority() const override
        {
            // Building shortcuts use 900 and intentionally take priority in
            // the colony planner.
            return 800;
        }

        bool HandleUIMessage(UTFWin::IWindow*, const UTFWin::Message& message) override
        {
            if (!message.IsType(UTFWin::kMsgKeyDown) &&
                !message.IsType(UTFWin::kMsgKeyDown2))
            {
                return false;
            }

            if (message.Key.modifiers != 0 ||
                message.Key.vkey < '0' || message.Key.vkey > '9')
            {
                return false;
            }

            Simulator::cSimulatorSpaceGame* game = Simulator::cSimulatorSpaceGame::Get();
            if (game == nullptr || game->GetUI() == nullptr || game->mpCommunityEditor != 0)
            {
                return false;
            }

            return DispatchToVisibleToolPanel(WindowManager.GetMainWindow(), message);
        }
    };

    IWinProcPtr sSpaceHotbarKeyboardProc;
    IWindowPtr sMainWindow;
}

void ERKEK2000QoL::InstallSpaceHotbarKeyboardShortcuts()
{
    if (sSpaceHotbarKeyboardProc != nullptr)
    {
        return;
    }

    UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
    if (mainWindow == nullptr)
    {
        SporeDebugPrint("ERKEK2000 QoL Runtime: main UI unavailable; Space hotbar shortcuts not installed.");
        return;
    }

    sMainWindow = mainWindow;
    sSpaceHotbarKeyboardProc = new SpaceHotbarKeyboardProc();
    mainWindow->AddWinProc(sSpaceHotbarKeyboardProc.get());
}

void ERKEK2000QoL::RemoveSpaceHotbarKeyboardShortcuts()
{
    if (sMainWindow != nullptr && sSpaceHotbarKeyboardProc != nullptr)
    {
        sMainWindow->RemoveWinProc(sSpaceHotbarKeyboardProc.get());
    }
    sSpaceHotbarKeyboardProc = nullptr;
    sMainWindow = nullptr;
}
