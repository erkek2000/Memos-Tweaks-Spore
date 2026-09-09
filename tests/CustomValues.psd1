@{
    # Build verification fixture; these are deliberately non-default values.
    RestoreVanillaBadgeRequirements = $true
    RestoreGroxExclusiveRadius      = $true
    RestoreGroxSpreadRadius         = $true
    RestoreCoreTravelRestriction    = $true
    RestoreGeneralSpiceProduction   = $true
    RestoreColonyBuildingCosts      = $true
    RestoreShieldCooldown           = $true
    InstantPlanetaryDialogue        = $true
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
    }
}
