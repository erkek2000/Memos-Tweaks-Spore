#include "stdafx.h"
#include "SpaceHotbarKeyboardShortcuts.h"

#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\App\IMessageManager.h>
#include <Spore\Simulator.h>
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
    UTFWin::IWindow* sAttachedWindow = nullptr;
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;

    void UpdateSpaceHotbarKeyboardProc()
    {
        // Same lifecycle rule as the other Space-only procedures: never touch
        // the UI tree while a stage is loading or outside Space.
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
        {
            return;
        }

        UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
        if (mainWindow == nullptr)
        {
            return;
        }

        if (sSpaceHotbarKeyboardProc == nullptr)
        {
            sSpaceHotbarKeyboardProc = new SpaceHotbarKeyboardProc();
        }

        if (sAttachedWindow != mainWindow)
        {
            mainWindow->AddWinProc(sSpaceHotbarKeyboardProc.get());
            sAttachedWindow = mainWindow;
            App::ConsolePrintF("ERKEK2000 QoL Runtime: space hotbar proc attached.");
        }
    }
}

void ERKEK2000QoL::InstallSpaceHotbarKeyboardShortcuts()
{
    if (sUpdateListener == nullptr)
        sUpdateListener = App::AddUpdateFunction(UpdateSpaceHotbarKeyboardProc);
}

void ERKEK2000QoL::RemoveSpaceHotbarKeyboardShortcuts()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
    sSpaceHotbarKeyboardProc = nullptr;
    sAttachedWindow = nullptr;
}
