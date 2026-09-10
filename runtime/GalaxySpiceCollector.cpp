#include "stdafx.h"
#include "GalaxySpiceCollector.h"

#include <algorithm>

#include <Spore\App\IMessageManager.h>
#include <Spore\Simulator.h>
#include <Spore\Simulator\SubSystem\GameModeManager.h>
#include <Spore\Simulator\cSimulatorPlayerUFO.h>
#include <Spore\Simulator\cSimulatorSpaceGame.h>
#include <Spore\Simulator\SubSystem\SpaceTrading.h>

namespace
{
    eastl::intrusive_ptr<App::UpdateMessageListener> sUpdateListener;
    Simulator::StarID sLastCollectedStar(0xFFFFFFFFu);

    size_t GetTradingObjectCount(const Simulator::cPlayerInventory* inventory,
        uint32_t itemID, bool& found)
    {
        size_t count = 0;
        found = false;
        for (const auto& item : inventory->mInventoryItems)
        {
            if (item != nullptr &&
                item->mItemType == Simulator::SpaceInventoryItemType::TradingObject &&
                item->mItemID.instanceID == itemID)
            {
                found = true;
                count += item->mItemCount;
            }
        }
        return count;
    }

    int CollectPlanetSpice(Simulator::cPlanetRecord* planet,
        Simulator::cEmpire* playerEmpire, Simulator::cPlayerInventory* inventory)
    {
        if (planet == nullptr || planet->IsDestroyed() ||
            planet->mSpiceGen.instanceID == 0 ||
            !Simulator::cPlanetRecord::HasControlledCity(planet, playerEmpire))
        {
            return 0;
        }

        const int stored = Simulator::cPlanetRecord::CalculateSpiceProduction(planet);
        if (stored <= 0)
        {
            return 0;
        }

        bool existingStack = false;
        const size_t before = GetTradingObjectCount(
            inventory, planet->mSpiceGen.instanceID, existingStack);
        if (!existingStack && inventory->GetAvailableCargoSlotsCount() == 0)
        {
            return 0;
        }

        const size_t configuredMaximum = inventory->mMaxItemCountPerItem > 0
            ? static_cast<size_t>(inventory->mMaxItemCountPerItem)
            : 999u;
        if (before >= configuredMaximum)
        {
            return 0;
        }

        const size_t room = configuredMaximum - before;
        const int requested = static_cast<int>(std::min<size_t>(
            static_cast<size_t>(stored), room));
        if (requested <= 0)
        {
            return 0;
        }

        Simulator::cSpaceTrading::Get()->ObtainTradingObject(planet->mSpiceGen, requested);

        bool foundAfter = false;
        const size_t after = GetTradingObjectCount(
            inventory, planet->mSpiceGen.instanceID, foundAfter);
        const int accepted = foundAfter && after > before
            ? static_cast<int>(std::min<size_t>(after - before, requested))
            : 0;

        // Remove only what the inventory demonstrably accepted. This order keeps
        // a full or rejected cargo operation from deleting production.
        if (accepted > 0)
        {
            Simulator::cPlanetRecord::CalculateSpiceProduction(planet, accepted);
        }
        return accepted;
    }

    void UpdateGalaxySpiceCollector()
    {
        if (!Simulator::IsSpaceGame() || Simulator::IsLoadingGameMode() ||
            Simulator::GetCurrentContext() != Simulator::SpaceContext::Galaxy)
        {
            sLastCollectedStar = Simulator::StarID(0xFFFFFFFFu);
            return;
        }

        Simulator::cSimulatorPlayerUFO* playerUFO = Simulator::cSimulatorPlayerUFO::Get();
        Simulator::cStarRecord* star = Simulator::GetActiveStarRecord();
        if (playerUFO == nullptr || playerUFO->GetUFO() == nullptr ||
            !playerUFO->GetUFO()->mbAtDestination || star == nullptr)
        {
            // Re-arm while travelling so returning to the same star collects
            // newly produced spice on a later visit.
            sLastCollectedStar = Simulator::StarID(0xFFFFFFFFu);
            return;
        }

        if (star->GetID() == sLastCollectedStar)
        {
            return;
        }
        sLastCollectedStar = star->GetID();

        Simulator::cEmpire* playerEmpire = Simulator::GetPlayerEmpire();
        Simulator::cSimulatorSpaceGame* spaceGame = Simulator::cSimulatorSpaceGame::Get();
        Simulator::cSpaceTrading* trading = Simulator::cSpaceTrading::Get();
        if (playerEmpire == nullptr || spaceGame == nullptr || trading == nullptr)
        {
            return;
        }

        Simulator::cPlayerInventory* inventory = spaceGame->GetPlayerInventory();
        if (inventory == nullptr)
        {
            return;
        }

        int totalCollected = 0;
        for (const auto& planet : star->GetPlanetRecords())
        {
            totalCollected += CollectPlanetSpice(planet.get(), playerEmpire, inventory);
        }

        if (totalCollected > 0)
        {
            App::ConsolePrintF("ERKEK2000 QoL Runtime: collected %d spice at galaxy-map star.",
                totalCollected);
        }
    }
}

void ERKEK2000QoL::InstallGalaxySpiceCollector()
{
    if (sUpdateListener == nullptr)
    {
        sUpdateListener = App::AddUpdateFunction(UpdateGalaxySpiceCollector);
    }
}

void ERKEK2000QoL::RemoveGalaxySpiceCollector()
{
    if (sUpdateListener != nullptr)
    {
        App::RemoveUpdateFunction(sUpdateListener);
        sUpdateListener = nullptr;
    }
    sLastCollectedStar = Simulator::StarID(0xFFFFFFFFu);
}
