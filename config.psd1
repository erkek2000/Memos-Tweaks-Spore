@{
    # These switches are applied when the package is rebuilt. They are not read
    # live by Spore, and changing them does not alter an already-built package.
    RestoreVanillaBadgeRequirements = $true
    RestoreGroxExclusiveRadius      = $true
    RestoreGroxSpreadRadius         = $true
    # Optional inherited Kisu galaxy-generation changes. These remain at
    # Kisu's values by default; enable a switch to use the value below.
    RestoreGroxEmpireSize           = $false
    RestoreTerrestrialMoonChance    = $false
    RestoreTerrestrialRingChance    = $false
    RestoreCoreTravelRestriction    = $true
    RestoreGeneralSpiceProduction   = $true
    # Kisu raises this from the patched vanilla limit of 5 to 10. Leave false
    # to retain 10, or enable it to use MaximumTradeRoutes below.
    RestoreVanillaMaximumTradeRoutes = $false
    # Kisu raises the per-transaction spice purchase cap from 200 to 999.
    # Leave false to retain 999, or enable it to use MaximumSpiceBought below.
    RestoreVanillaMaximumSpiceBought = $false
    # Kisu raises passive storage per colony from 5 to 15. Leave false to
    # retain 15, or enable it to use ColonySpiceStorage below.
    RestoreVanillaColonySpiceStorage = $false
    RestoreColonyBuildingCosts      = $true
    RestoreShieldCooldown           = $true
    InstantPlanetaryDialogue        = $true

    # Runtime DLL switches. These are compiled into the DLL by
    # build-runtime.ps1; they are not live settings.
    PreventBioDisastersWithBioProtector = $true
    CloseDialogueWithEscape             = $true
    CloseDialogueWithTab                = $true
    CollectSpiceAtGalaxyStars           = $true
    EnforceCargoStackLimit              = $true
    BuildingKeyboardShortcuts           = $true
    SpaceHotbarKeyboardShortcuts        = $true

    # Values used when the corresponding switch above is enabled.
    Values = @{
        GroxExclusiveRadius       = 100.0
        GroxSpreadRadius          = 105.0
        CoreTravelMinimum         = 70.0
        CoreTravelMaximum         = 100.0
        SpiceProductionMultiplier = 0.0005
        HouseCost                 = 25600.0
        EntertainmentCost         = 12800.0
        FactoryCost               = 19200.0
        TurretCost                = 16000.0
        ShieldCooldownSeconds     = 180.0
        CargoStackLimit           = 999
        GroxEmpireSize            = 2400
        TerrestrialMoonChance     = 0.01
        TerrestrialRingChance     = 0.1
        MaximumTradeRoutes        = 5
        MaximumSpiceBought        = 200
        ColonySpiceStorage        = 5
    }
}
