#include "stdafx.h"
#include "BuildingKeyboardShortcuts.h"

#include <Spore\Palettes\PalettePageUI.h>
#include <Spore\App\IMessageManager.h>
#include <Spore\Simulator.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
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

    bool ClickPaletteItem(UTFWin::IWindow* window, uint32_t itemID, uint32_t alternateItemID = 0)
    {
        if (window == nullptr)
        {
            return false;
        }

        for (UTFWin::IWinProc* procedure : window->procedures())
        {
            Palettes::PalettePageUI* page = object_cast<Palettes::PalettePageUI>(procedure);
            if (page == nullptr)
            {
                continue;
            }

            for (const StandardItemUIPtr& itemUI : page->mStandardItems)
            {
                if (itemUI == nullptr || itemUI->mpItem == nullptr)
                {
                    continue;
                }

                const uint32_t candidateID = itemUI->mpItem->mName.instanceID;
                if (candidateID != itemID &&
                    (alternateItemID == 0 || candidateID != alternateItemID))
                {
                    continue;
                }

                UTFWin::IWindow* itemWindow = itemUI->mpWindow.get();
                if (!IsUsable(itemWindow))
                {
                    continue;
                }

                UTFWin::Message click{};
                click.source = itemWindow;
                click.eventType = UTFWin::kMsgButtonClick;
                WindowManager.SendMsg(itemWindow, itemWindow, click, true);
                return true;
            }
        }

        for (UTFWin::IWindow* child : window->children())
        {
            if (ClickPaletteItem(child, itemID, alternateItemID))
            {
                return true;
            }
        }
        return false;
    }

    class BuildingKeyboardProc final : public UTFWin::DefaultWinProc<UTFWin::kEventFlagBasicInput>
    {
    public:
        int GetPriority() const override
        {
            return 900;
        }

        bool HandleUIMessage(UTFWin::IWindow*, const UTFWin::Message& message) override
        {
            if (!message.IsType(UTFWin::kMsgKeyDown) &&
                !message.IsType(UTFWin::kMsgKeyDown2))
            {
                return false;
            }

            // Do not consume modified number keys or interfere outside the
            // active Space-stage colony planner.
            if (message.Key.modifiers != 0 ||
                Simulator::cSimulatorSpaceGame::Get() == nullptr ||
                SimulatorSpaceGame.mpCommunityEditor == 0)
            {
                return false;
            }

            uint32_t itemID = 0;
            uint32_t alternateItemID = 0;
            switch (message.Key.vkey)
            {
            case '1': itemID = id("house"); break;
            case '2': itemID = id("entertainment"); break;
            case '3': itemID = id("factory"); break;
            case '4':
                itemID = id("Space_turret");
                alternateItemID = id("turret");
                break;
            default:
                return false;
            }

            UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
            return ClickPaletteItem(mainWindow, itemID, alternateItemID);
        }
    };

    IWinProcPtr sBuildingKeyboardProc;
    UTFWin::IWindow* sAttachedWindow = nullptr;
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;

    void UpdateBuildingKeyboardProc()
    {
        // Never touch the UI tree while a stage is loading or outside Space.
        // The main window and its procedure list are only stable once the
        // Space game is fully entered; attaching during a transition is what
        // crashed 0.5.1 through 0.5.3.
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
        {
            return;
        }

        UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
        if (mainWindow == nullptr)
        {
            return;
        }

        if (sBuildingKeyboardProc == nullptr)
        {
            sBuildingKeyboardProc = new BuildingKeyboardProc();
        }

        // Attach once per live main-window identity. The procedure is kept
        // alive for the whole session and is never removed again: the game
        // does not own it, and RemoveWinProc during a UI transition is the
        // corruption vector this avoids.
        if (sAttachedWindow != mainWindow)
        {
            mainWindow->AddWinProc(sBuildingKeyboardProc.get());
            sAttachedWindow = mainWindow;
            App::ConsolePrintF("ERKEK2000 QoL Runtime: building shortcut proc attached.");
        }
    }
}

void ERKEK2000QoL::InstallBuildingKeyboardShortcuts()
{
    if (sUpdateListener == nullptr)
        sUpdateListener = App::AddUpdateFunction(UpdateBuildingKeyboardProc);
}

void ERKEK2000QoL::RemoveBuildingKeyboardShortcuts()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
    // The game tears the UI down during shutdown and never owned the
    // procedure. Do not call RemoveWinProc on exit.
    sBuildingKeyboardProc = nullptr;
    sAttachedWindow = nullptr;
}
