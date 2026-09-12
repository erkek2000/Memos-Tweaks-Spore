@{
    # Crash-isolation preset: CARGO STACK LIMIT ONLY. Isolates the live
    # SetMaxCargoAmount update; all other runtime features are off.
    RestoreVanillaBadgeRequirements = $true
    RestoreGroxExclusiveRadius      = $true
    RestoreGroxSpreadRadius         = $true
    RestoreGroxEmpireSize           = $false
    RestoreTerrestrialMoonChance    = $false
    RestoreTerrestrialRingChance    = $false
    RestoreCoreTravelRestriction    = $true
    RestoreGeneralSpiceProduction   = $true
    RestoreVanillaHomeworldSpiceProduction = $false
    RestoreVanillaMaximumTradeRoutes = $false
    RestoreVanillaMaximumSpiceBought = $false
    RestoreVanillaColonySpiceStorage = $false
    RestoreVanillaSpiceStorageCooldown = $false
    RestoreVanillaHappinessBoosterCooldown = $false
    RestoreVanillaLoyaltyBoosterCooldown = $false
    RestoreVanillaUberTurretCooldown = $false
    RestoreVanillaEmbassyCooldown = $false
    RestoreColonyBuildingCosts      = $true
    RestoreShieldCooldown           = $true
    InstantPlanetaryDialogue        = $true

    PreventBioDisastersWithBioProtector = $false
    CloseDialogueWithEscape             = $false
    CloseDialogueWithTab                = $false
    CloseDialogueWithSpacebar            = $false
    FastDialogueOpening                 = $false
    CollectSpiceAtGalaxyStars           = $false
    EnforceCargoStackLimit              = $true
    BuildingKeyboardShortcuts           = $false
    SpaceHotbarKeyboardShortcuts        = $false
    ColonyPatternButtons                = $false
    CropCircleUplift                    = $false

    Values = @{
        GroxExclusiveRadius       = 100.0
        GroxSpreadRadius          = 105.0
        CoreTravelMinimum         = 70.0
        CoreTravelMaximum         = 100.0
        SpiceProductionMultiplier = 0.0005
        HomeworldSpiceProductionMultiplier = 0.025
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
        SpiceStorageCooldownSeconds = 30.0
        HappinessBoosterCooldownSeconds = 30.0
        LoyaltyBoosterCooldownSeconds = 30.0
        UberTurretCooldownSeconds = 30.0
        EmbassyCooldownSeconds = 30.0
        CropCircleUpliftIntervalSeconds = 1200
    }
}
