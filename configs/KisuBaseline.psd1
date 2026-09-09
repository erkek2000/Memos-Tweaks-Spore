@{
    # Verification preset: retain Kisu's values and omit ERKEK-specific UI.
    RestoreVanillaBadgeRequirements = $false
    RestoreGroxExclusiveRadius      = $false
    RestoreGroxSpreadRadius         = $false
    RestoreGroxEmpireSize           = $false
    RestoreTerrestrialMoonChance    = $false
    RestoreTerrestrialRingChance    = $false
    RestoreCoreTravelRestriction    = $false
    RestoreGeneralSpiceProduction   = $false
    RestoreVanillaMaximumTradeRoutes = $false
    RestoreVanillaMaximumSpiceBought = $false
    RestoreVanillaColonySpiceStorage = $false
    RestoreColonyBuildingCosts      = $false
    RestoreShieldCooldown           = $false
    InstantPlanetaryDialogue        = $false
    PreventBioDisastersWithBioProtector = $false
    CloseDialogueWithEscape             = $false
    CloseDialogueWithTab                = $false
    CollectSpiceAtGalaxyStars           = $false
    EnforceCargoStackLimit              = $false
    BuildingKeyboardShortcuts           = $false
    SpaceHotbarKeyboardShortcuts        = $false
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
