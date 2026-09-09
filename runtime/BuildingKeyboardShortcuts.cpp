#include "stdafx.h"
#include "BuildingKeyboardShortcuts.h"

#include <Spore\Palettes\PalettePageUI.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
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
    IWindowPtr sMainWindow;
}

void ERKEK2000QoL::InstallBuildingKeyboardShortcuts()
{
    if (sBuildingKeyboardProc != nullptr)
    {
        return;
    }

    UTFWin::IWindow* mainWindow = WindowManager.GetMainWindow();
    if (mainWindow == nullptr)
    {
        SporeDebugPrint("ERKEK2000 QoL Runtime: main UI unavailable; building shortcuts not installed.");
        return;
    }

    sMainWindow = mainWindow;
    sBuildingKeyboardProc = new BuildingKeyboardProc();
    mainWindow->AddWinProc(sBuildingKeyboardProc.get());
}

void ERKEK2000QoL::RemoveBuildingKeyboardShortcuts()
{
    if (sMainWindow != nullptr && sBuildingKeyboardProc != nullptr)
    {
        sMainWindow->RemoveWinProc(sBuildingKeyboardProc.get());
    }
    sBuildingKeyboardProc = nullptr;
    sMainWindow = nullptr;
}
