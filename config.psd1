@{
    # These switches are applied when the package is rebuilt. They are not read
    # live by Spore, and changing them does not alter an already-built package.
    RestoreVanillaBadgeRequirements = $true
    RestoreGroxExclusiveRadius      = $true
    RestoreGroxSpreadRadius         = $true
    RestoreCoreTravelRestriction    = $true
    RestoreGeneralSpiceProduction   = $true
    RestoreColonyBuildingCosts      = $true
    RestoreShieldCooldown           = $true
    InstantPlanetaryDialogue        = $true

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
    }
}
