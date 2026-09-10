#include "stdafx.h"
#include "BuildingKeyboardShortcuts.h"
#include "CommKeyboardShortcuts.h"
#include "CargoStackLimit.h"
#include "ColonyPattern.h"
#include "CropCircleUplift.h"
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
            App::ConsolePrintF("ERKEK2000 QoL Runtime: prevented eco-disaster on Bio Protector planet.");
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
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB || ERKEK_FAST_DIALOGUE_OPENING
    ERKEK2000QoL::InstallCommKeyboardShortcuts();
#endif
#if ERKEK_COLLECT_GALAXY_SPICE
    ERKEK2000QoL::InstallGalaxySpiceCollector();
#endif
#if ERKEK_ENFORCE_CARGO_STACK_LIMIT
    ERKEK2000QoL::InstallCargoStackLimit();
#endif
#if ERKEK_COLONY_PATTERN_BUTTONS
    ERKEK2000QoL::InstallColonyPatternButtons();
#endif
#if ERKEK_CROP_CIRCLE_UPLIFT
    ERKEK2000QoL::InstallCropCircleUplift();
#endif
}

void Dispose()
{
#if ERKEK_CROP_CIRCLE_UPLIFT
    ERKEK2000QoL::RemoveCropCircleUplift();
#endif
#if ERKEK_COLONY_PATTERN_BUTTONS
    ERKEK2000QoL::RemoveColonyPatternButtons();
#endif
#if ERKEK_SPACE_HOTBAR_SHORTCUTS
    ERKEK2000QoL::RemoveSpaceHotbarKeyboardShortcuts();
#endif
#if ERKEK_ENFORCE_CARGO_STACK_LIMIT
    ERKEK2000QoL::RemoveCargoStackLimit();
#endif
#if ERKEK_COLLECT_GALAXY_SPICE
    ERKEK2000QoL::RemoveGalaxySpiceCollector();
#endif
#if ERKEK_CLOSE_WITH_ESCAPE || ERKEK_CLOSE_WITH_TAB || ERKEK_FAST_DIALOGUE_OPENING
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
#if ERKEK_CROP_CIRCLE_UPLIFT
    ERKEK2000QoL::AttachCropCircleUpliftDetour();
#endif
#if ERKEK_FAST_DIALOGUE_OPENING
    ERKEK2000QoL::AttachDialogueSpeedDetour();
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
