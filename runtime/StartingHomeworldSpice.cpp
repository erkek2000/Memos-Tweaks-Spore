#include "stdafx.h"
#include "StartingHomeworldSpice.h"

#include <Spore\App\Property.h>
#include <Spore\Simulator\cPlanet.h>
#include <Spore\Simulator\cPlanetRecord.h>
#include <Spore\Simulator\SubSystem\SpaceTrading.h>
#include <Spore\Simulator\SubSystem\StarManager.h>

#include <vector>

namespace
{
    uint32_t SpiceColorPropertyID()
    {
        return 0x058CBB75; // spaceEconomySpiceColor
    }

    void SetPlanetSpiceColor(Simulator::cPlanetRecord* record, const ResourceKey& spice)
    {
        cPlanetPtr planet;
        StarManager.RecordToPlanet(record, planet);
        if (planet == nullptr) return;

        PropertyListPtr properties;
        uint32_t color = 0;
        if (PropManager.GetPropertyList(spice.instanceID, GroupIDs::SpaceTrading_, properties) &&
            App::Property::GetUInt32(properties.get(), SpiceColorPropertyID(), color))
            planet->mSpaceEconomySpiceColor = color;
    }

    // The assignment hook is reached during new-homeworld creation before a
    // spice has been stored. Existing saves always have mSpiceGen populated,
    // so they fall through to the native function unchanged.
    member_detour(RandomizeNewHomeworldSpice, Simulator::cSpaceTrading,
        void(Simulator::cPlanetRecord*, bool))
    {
        void detoured(Simulator::cPlanetRecord* record, bool forceReassign)
        {
            const bool isNewHomeworld = record != nullptr && record->mbHomeWorld &&
                record->mSpiceGen.instanceID == 0 && !forceReassign;
            original_function(this, record, forceReassign);
            if (!isNewHomeworld || record == nullptr || this->mSpices.empty()) return;

            // The native homeworld assignment is normally red. Choose from
            // the same valid spice list, excluding that result whenever a
            // different choice exists, so a new game never silently repeats
            // the default colour.
            std::vector<ResourceKey> candidates;
            candidates.reserve(this->mSpices.size());
            for (const ResourceKey& spice : this->mSpices)
            {
                if (spice.instanceID != record->mSpiceGen.instanceID)
                    candidates.push_back(spice);
            }
            if (candidates.empty()) candidates.assign(this->mSpices.begin(), this->mSpices.end());

            const uint64_t seed = GetTickCount64() ^
                static_cast<uint64_t>(record->GetID().internalValue);
            const ResourceKey selected = candidates[static_cast<size_t>(seed % candidates.size())];
            record->mSpiceGen = selected;
            SetPlanetSpiceColor(record, selected);
            App::ConsolePrintF("ERKEK2000 QoL Runtime: randomized spice for a newly created homeworld.");
        }
    };
}

void ERKEK2000QoL::AttachStartingHomeworldSpiceDetour()
{
    RandomizeNewHomeworldSpice::attach(GetAddress(Simulator::cSpaceTrading, AssignPlanetSpice));
}
