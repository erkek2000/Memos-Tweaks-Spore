@{
    # Verification preset: retain Kisu's values and omit ERKEK-specific UI.
    RestoreVanillaBadgeRequirements = $false
    RestoreGroxExclusiveRadius      = $false
    RestoreGroxSpreadRadius         = $false
    RestoreCoreTravelRestriction    = $false
    RestoreGeneralSpiceProduction   = $false
    RestoreColonyBuildingCosts      = $false
    RestoreShieldCooldown           = $false
    InstantPlanetaryDialogue        = $false
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
    }
}
