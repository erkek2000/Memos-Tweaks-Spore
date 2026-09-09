#include "stdafx.h"
#include "BuildingKeyboardShortcuts.h"
#include "CommKeyboardShortcuts.h"
#include "CargoStackLimit.h"
#include "GalaxySpiceCollector.h"
#include "SpaceHotbarKeyboardShortcuts.h"

namespace
{
    bool HasBioProtector(const Simulator::cPlanetRecord* planet)
    {
        if (planet == nullptr) return false;

        const uint32_t bioProtectorModelID = id("SG_colonytool_bioprotector");
        for (const auto& object : planet->mPlanetObjects)
        {
            if (object.mKey.instanceID == bioProtectorModelID)
            {
                return true;
            }
        }
        return false;
    }
}

member_detour(PreventBioDisasterWithProtector, Simulator::cMissionManager,
    Simulator::cMission*(uint32_t, Simulator::cPlanetRecord*, Simulator::cEmpire*, Simulator::cMission*))
{
    Simulator::cMission* detoured(uint32_t missionID,
        Simulator::cPlanetRecord* sourcePlanet,
        Simulator::cEmpire* ownerEmpire,
        Simulator::cMission* parentMission)
    {
        if (missionID == id("biospherecollapse") && HasBioProtector(sourcePlanet))
        {
            SporeDebugPrint("ERKEK2000 QoL Runtime: prevented eco-disaster on Bio Protector planet.");
            return nullptr;
        }

        return original_function(this, missionID, sourcePlanet, ownerEmpire, parentMission);
    }
};

void Initialize()
{
#if ERKEK_BUILDING_SHORTCUTS
    ERKEK2000QoL::InstallBuildingKeyboardShortcuts();
#endif
#if ERKEK_SPACE_HOTBAR_SHORTCUTS
    ERKEK2000QoL::InstallSpaceHotbarKeyboardShortcuts();
#endif
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB
    ERKEK2000QoL::InstallCommKeyboardShortcuts();
#endif
#if ERKEK_COLLECT_GALAXY_SPICE
    ERKEK2000QoL::InstallGalaxySpiceCollector();
#endif
#if ERKEK_ENFORCE_CARGO_STACK_LIMIT
    ERKEK2000QoL::InstallCargoStackLimit();
#endif
}

void Dispose()
{
#if ERKEK_SPACE_HOTBAR_SHORTCUTS
    ERKEK2000QoL::RemoveSpaceHotbarKeyboardShortcuts();
#endif
#if ERKEK_ENFORCE_CARGO_STACK_LIMIT
    ERKEK2000QoL::RemoveCargoStackLimit();
#endif
#if ERKEK_COLLECT_GALAXY_SPICE
    ERKEK2000QoL::RemoveGalaxySpiceCollector();
#endif
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB
    ERKEK2000QoL::RemoveCommKeyboardShortcuts();
#endif
#if ERKEK_BUILDING_SHORTCUTS
    ERKEK2000QoL::RemoveBuildingKeyboardShortcuts();
#endif
}

void AttachDetours()
{
#if ERKEK_PREVENT_BIO_DISASTERS
    PreventBioDisasterWithProtector::attach(
        GetAddress(Simulator::cMissionManager, CreateMission));
#endif
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        ModAPI::AddPostInitFunction(Initialize);
        ModAPI::AddDisposeFunction(Dispose);
        PrepareDetours(module);
        AttachDetours();
        CommitDetours();
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
