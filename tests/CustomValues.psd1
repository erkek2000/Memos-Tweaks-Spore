@{
    # Build verification fixture; these are deliberately non-default values.
    RestoreVanillaBadgeRequirements = $true
    RestoreGroxExclusiveRadius      = $true
    RestoreGroxSpreadRadius         = $true
    RestoreGroxEmpireSize           = $true
    RestoreTerrestrialMoonChance    = $true
    RestoreTerrestrialRingChance    = $true
    RestoreCoreTravelRestriction    = $true
    RestoreGeneralSpiceProduction   = $true
    RestoreVanillaMaximumTradeRoutes = $true
    RestoreVanillaMaximumSpiceBought = $true
    RestoreVanillaColonySpiceStorage = $true
    RestoreColonyBuildingCosts      = $true
    RestoreShieldCooldown           = $true
    InstantPlanetaryDialogue        = $true
    PreventBioDisastersWithBioProtector = $true
    CloseDialogueWithEscape             = $true
    CloseDialogueWithTab                = $true
    CollectSpiceAtGalaxyStars           = $true
    EnforceCargoStackLimit              = $true
    BuildingKeyboardShortcuts           = $true
    SpaceHotbarKeyboardShortcuts        = $true
    Values = @{
        GroxExclusiveRadius       = 90.0
        GroxSpreadRadius          = 110.0
        CoreTravelMinimum         = 60.0
        CoreTravelMaximum         = 95.0
        SpiceProductionMultiplier = 0.0006
        HouseCost                 = 24000.0
        EntertainmentCost         = 12000.0
        FactoryCost               = 18000.0
        TurretCost                = 15000.0
        ShieldCooldownSeconds     = 150.0
        CargoStackLimit           = 321
        GroxEmpireSize            = 1200
        TerrestrialMoonChance     = 0.05
        TerrestrialRingChance     = 0.2
        MaximumTradeRoutes        = 7
        MaximumSpiceBought        = 432
        ColonySpiceStorage        = 9
    }
}
