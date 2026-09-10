#include "stdafx.h"
#include "CargoStackLimit.h"

#include <Spore\App\IMessageManager.h>
#include <Spore\Simulator.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>

namespace
{
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;

    void UpdateCargoStackLimit()
    {
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode())
        {
            return;
        }

        Simulator::cSimulatorSpaceGame* spaceGame = Simulator::cSimulatorSpaceGame::Get();
        Simulator::cPlayerInventory* inventory = spaceGame == nullptr
            ? nullptr
            : spaceGame->GetPlayerInventory();
        if (inventory != nullptr &&
            inventory->mMaxItemCountPerItem != ERKEK_CARGO_STACK_LIMIT)
        {
            inventory->SetMaxCargoAmount(ERKEK_CARGO_STACK_LIMIT);
            App::ConsolePrintF("ERKEK2000 QoL Runtime: cargo stack limit set to %d.",
                ERKEK_CARGO_STACK_LIMIT);
        }
    }
}

void ERKEK2000QoL::InstallCargoStackLimit()
{
    if (sUpdateListener == nullptr)
    {
        sUpdateListener = App::AddUpdateFunction(UpdateCargoStackLimit);
    }
}

void ERKEK2000QoL::RemoveCargoStackLimit()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
}
