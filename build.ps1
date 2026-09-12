param(
    [string]$ToolDirectory = (Join-Path $PSScriptRoot '..\..\Tools\SporeModderFX'),
    [string]$ConfigurationFile = (Join-Path $PSScriptRoot 'config.psd1')
)
$ErrorActionPreference = 'Stop'
function Get-Sha256FileRecord([string]$Path) {
    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $hash = [System.BitConverter]::ToString($sha256.ComputeHash($stream)).Replace('-', '')
    } finally {
        $stream.Dispose()
        $sha256.Dispose()
    }
    [pscustomobject]@{ Hash = $hash; Path = $Path }
}
$ToolDirectory = (Resolve-Path -LiteralPath $ToolDirectory).Path
$sourceProject = Join-Path $PSScriptRoot 'project'
$dist = Join-Path $PSScriptRoot 'dist'
$reports = Join-Path $PSScriptRoot 'reports'
$package = Join-Path $dist 'ERKEK2000_QoL_Runtime.package'
$smfx = Join-Path $ToolDirectory 'smfx.exe'
$jar = Join-Path $ToolDirectory 'SporeModderFX.jar'
$kisu = Join-Path $PSScriptRoot '..\..\KisuTweaks-1.5\KisuTweaks-1.5\KisuTweaks.package'
$commReference = Join-Path $PSScriptRoot 'reference\vanilla\PatchData-CommScreen-3.spui'
$kisuReference = Join-Path $PSScriptRoot 'reference\kisu'
$ConfigurationFile = (Resolve-Path -LiteralPath $ConfigurationFile).Path
$config = @{}
Import-LocalizedData -BindingVariable config `
    -BaseDirectory (Split-Path -Parent $ConfigurationFile) `
    -FileName ([System.IO.Path]::GetFileNameWithoutExtension($ConfigurationFile)) `
    -UICulture 'en-US'
$knownOptions = @(
    'RestoreVanillaBadgeRequirements',
    'RestoreGroxExclusiveRadius',
    'RestoreGroxSpreadRadius',
    'RestoreGroxEmpireSize',
    'RestoreTerrestrialMoonChance',
    'RestoreTerrestrialRingChance',
    'RestoreCoreTravelRestriction',
    'RestoreGeneralSpiceProduction',
    'RestoreVanillaHomeworldSpiceProduction',
    'RestoreVanillaMaximumTradeRoutes',
    'RestoreVanillaMaximumSpiceBought',
    'RestoreVanillaColonySpiceStorage',
    'RestoreVanillaSpiceStorageCooldown',
    'RestoreVanillaHappinessBoosterCooldown',
    'RestoreVanillaLoyaltyBoosterCooldown',
    'RestoreVanillaUberTurretCooldown',
    'RestoreVanillaEmbassyCooldown',
    'RestoreColonyBuildingCosts',
    'RestoreShieldCooldown',
    'InstantPlanetaryDialogue',
    'PreventBioDisastersWithBioProtector',
    'CloseDialogueWithEscape',
    'CloseDialogueWithTab',
    'CloseDialogueWithSpacebar',
    'CloseDialogueWithEnd',
    'FastDialogueOpening',
    'CollectSpiceAtGalaxyStars',
    'EnforceCargoStackLimit',
    'BuildingKeyboardShortcuts',
    'SpaceHotbarKeyboardShortcuts',
    'ColonyPatternButtons',
    'CropCircleUplift',
    'Values'
)
foreach ($option in $knownOptions | Where-Object { $_ -ne 'Values' }) {
    if (!$config.ContainsKey($option) -or $config[$option] -isnot [bool]) {
        throw "Configuration option '$option' must be present and Boolean."
    }
}
$knownValues = @(
    'GroxExclusiveRadius', 'GroxSpreadRadius', 'CoreTravelMinimum',
    'CoreTravelMaximum', 'SpiceProductionMultiplier',
    'HomeworldSpiceProductionMultiplier', 'HouseCost',
    'EntertainmentCost', 'FactoryCost', 'TurretCost', 'ShieldCooldownSeconds',
    'CargoStackLimit', 'GroxEmpireSize', 'TerrestrialMoonChance',
    'TerrestrialRingChance', 'MaximumTradeRoutes', 'MaximumSpiceBought',
    'ColonySpiceStorage', 'SpiceStorageCooldownSeconds',
    'HappinessBoosterCooldownSeconds', 'LoyaltyBoosterCooldownSeconds',
    'UberTurretCooldownSeconds', 'EmbassyCooldownSeconds',
    'CropCircleUpliftIntervalSeconds'
)
if ($config.Values -isnot [hashtable]) { throw "Configuration option 'Values' must be a hashtable." }
$unknownValues = @($config.Values.Keys | Where-Object { $_ -notin $knownValues })
if ($unknownValues.Count -ne 0) { throw "Unknown configured value(s): $($unknownValues -join ', ')" }
foreach ($valueName in $knownValues) {
    if (!$config.Values.ContainsKey($valueName) -or $config.Values[$valueName] -is [bool] -or
            $config.Values[$valueName] -isnot [ValueType]) {
        throw "Configuration value '$valueName' must be numeric."
    }
    if ([double]$config.Values[$valueName] -lt 0) {
        throw "Configuration value '$valueName' cannot be negative."
    }
}
if ([double]$config.Values.CoreTravelMinimum -gt [double]$config.Values.CoreTravelMaximum) {
    throw 'CoreTravelMinimum cannot exceed CoreTravelMaximum.'
}
if ([int]$config.Values.CargoStackLimit -ne [double]$config.Values.CargoStackLimit -or
        [int]$config.Values.CargoStackLimit -lt 1) {
    throw 'CargoStackLimit must be a positive whole number.'
}
if ([int]$config.Values.GroxEmpireSize -ne [double]$config.Values.GroxEmpireSize -or
        [int]$config.Values.GroxEmpireSize -lt 1) {
    throw 'GroxEmpireSize must be a positive whole number.'
}
if ([int]$config.Values.MaximumTradeRoutes -ne [double]$config.Values.MaximumTradeRoutes -or
        [int]$config.Values.MaximumTradeRoutes -lt 1) {
    throw 'MaximumTradeRoutes must be a positive whole number.'
}
if ([int]$config.Values.MaximumSpiceBought -ne [double]$config.Values.MaximumSpiceBought -or
        [int]$config.Values.MaximumSpiceBought -lt 1) {
    throw 'MaximumSpiceBought must be a positive whole number.'
}
if ([int]$config.Values.ColonySpiceStorage -ne [double]$config.Values.ColonySpiceStorage -or
        [int]$config.Values.ColonySpiceStorage -lt 1) {
    throw 'ColonySpiceStorage must be a positive whole number.'
}
if ([int]$config.Values.CropCircleUpliftIntervalSeconds -ne [double]$config.Values.CropCircleUpliftIntervalSeconds -or
        [int]$config.Values.CropCircleUpliftIntervalSeconds -lt 1) {
    throw 'CropCircleUpliftIntervalSeconds must be a positive whole number.'
}
foreach ($chanceName in @('TerrestrialMoonChance', 'TerrestrialRingChance')) {
    if ([double]$config.Values[$chanceName] -gt 1) {
        throw "$chanceName must be between 0 and 1."
    }
}
$unknownOptions = @($config.Keys | Where-Object { $_ -notin $knownOptions })
if ($unknownOptions.Count -ne 0) {
    throw "Unknown configuration option(s): $($unknownOptions -join ', ')"
}

function Set-PropertyLine {
    param([string]$Path, [string]$Property, [string]$Line)
    $content = Get-Content -LiteralPath $Path -Raw
    $pattern = "(?m)^\s*\S+\s+$([regex]::Escape($Property))\s+.*$"
    $matches = [regex]::Matches($content, $pattern)
    if ($matches.Count -ne 1) {
        throw "Expected one '$Property' property in $Path; found $($matches.Count)."
    }
    $updated = [regex]::Replace($content, $pattern, $Line)
    [System.IO.File]::WriteAllText($Path, $updated, (New-Object System.Text.UTF8Encoding($false)))
}

New-Item -ItemType Directory -Force -Path $dist, $reports | Out-Null
$stageRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("erkek2000-qol-" + [guid]::NewGuid().ToString('N'))
$project = Join-Path $stageRoot 'project'
$packageCandidate = Join-Path $stageRoot 'ERKEK2000_QoL.package'
New-Item -ItemType Directory -Path $project | Out-Null
Copy-Item -Path (Join-Path $sourceProject '*') -Destination $project -Recurse -Force
$commProject = Join-Path $project 'layouts_atlas~\CommScreen-3.spui'

if (!$config.RestoreVanillaBadgeRequirements) {
    Copy-Item -Path (Join-Path $kisuReference 'space_badges~\*') `
        -Destination (Join-Path $project 'space_badges~') -Force
}

$solarSystem = Join-Path $project 'gametuning~\SpaceSolarSystem.prop.prop_t'
Set-PropertyLine $solarSystem 'grobEmpireSize' $(if ($config.RestoreGroxEmpireSize) {
    "int32 grobEmpireSize $($config.Values.GroxEmpireSize)"
} else { 'int32 grobEmpireSize 300' })
Set-PropertyLine $solarSystem 'chanceTerrestrialHasMoon' $(if ($config.RestoreTerrestrialMoonChance) {
    "float chanceTerrestrialHasMoon $($config.Values.TerrestrialMoonChance)"
} else { 'float chanceTerrestrialHasMoon 0.1' })
Set-PropertyLine $solarSystem 'chanceTerrestrialHasRings' $(if ($config.RestoreTerrestrialRingChance) {
    "float chanceTerrestrialHasRings $($config.Values.TerrestrialRingChance)"
} else { 'float chanceTerrestrialHasRings 0.25' })
Set-PropertyLine $solarSystem 'grobOnlyRadius' $(if ($config.RestoreGroxExclusiveRadius) {
    "float grobOnlyRadius $($config.Values.GroxExclusiveRadius)"
} else { 'float grobOnlyRadius 30' })
Set-PropertyLine $solarSystem 'grobStarsSpreadRadius' $(if ($config.RestoreGroxSpreadRadius) {
    "float grobStarsSpreadRadius $($config.Values.GroxSpreadRadius)"
} else { 'float grobStarsSpreadRadius 35' })

$galacticConstants = Join-Path $project 'gametuning~\SpaceGalacticConstants.prop.prop_t'
Set-PropertyLine $galacticConstants 'galacticCoreTravelRadii' $(if ($config.RestoreCoreTravelRestriction) {
    "vector2 galacticCoreTravelRadii ($($config.Values.CoreTravelMinimum), $($config.Values.CoreTravelMaximum))"
} else { 'vector2 galacticCoreTravelRadii (25, 35)' })

$economy = Join-Path $project 'gametuning~\SpaceEconomy.prop.prop_t'
Set-PropertyLine $economy 'spaceEconomySpiceProductionMultiplier' $(if ($config.RestoreGeneralSpiceProduction) {
    "float spaceEconomySpiceProductionMultiplier $($config.Values.SpiceProductionMultiplier)"
} else { 'float spaceEconomySpiceProductionMultiplier 0.00075' })
Set-PropertyLine $economy 'spaceEconomySpiceHomeworldMultiplier' $(if ($config.RestoreVanillaHomeworldSpiceProduction) {
    "float spaceEconomySpiceHomeworldMultiplier $($config.Values.HomeworldSpiceProductionMultiplier)"
} else { 'float spaceEconomySpiceHomeworldMultiplier 0.25' })
Set-PropertyLine $economy 'tradeRouteMaxNumber' $(if ($config.RestoreVanillaMaximumTradeRoutes) {
    "int32 tradeRouteMaxNumber $($config.Values.MaximumTradeRoutes)"
} else { 'int32 tradeRouteMaxNumber 10' })
Set-PropertyLine $economy 'spaceEconomyTradeMaxSpiceBought' $(if ($config.RestoreVanillaMaximumSpiceBought) {
    "int32 spaceEconomyTradeMaxSpiceBought $($config.Values.MaximumSpiceBought)"
} else { 'int32 spaceEconomyTradeMaxSpiceBought 999' })

$colonization = Join-Path $project 'gametuning~\SpaceColonization.prop.prop_t'
Set-PropertyLine $colonization 'colonyMaxSpiceStoredPerColony' $(if ($config.RestoreVanillaColonySpiceStorage) {
    "int32 colonyMaxSpiceStoredPerColony $($config.Values.ColonySpiceStorage)"
} else { 'int32 colonyMaxSpiceStoredPerColony 15' })

$buildingTuning = Join-Path $project 'animations~\CityGameBuildingTuning.prop.prop_t'
$buildingValues = if ($config.RestoreColonyBuildingCosts) {
    @{
        DefenseCostSpace = $config.Values.TurretCost
        EntertainmentCostSpace = $config.Values.EntertainmentCost
        IndustryCostSpace = $config.Values.FactoryCost
        SleepCostSpace = $config.Values.HouseCost
    }
} else {
    @{ DefenseCostSpace = 10000; EntertainmentCostSpace = 8000; IndustryCostSpace = 12000; SleepCostSpace = 16000 }
}
foreach ($property in $buildingValues.Keys) {
    Set-PropertyLine $buildingTuning $property "float $property $($buildingValues[$property])"
}

$shield = Join-Path $project 'spacetools~\shield.prop.prop_t'
Set-PropertyLine $shield 'spaceToolRechargeRate' $(if ($config.RestoreShieldCooldown) {
    "float spaceToolRechargeRate $($config.Values.ShieldCooldownSeconds)"
} else { 'float spaceToolRechargeRate 120' })

$spiceStorageTool = Join-Path $project 'spacetools~\placespicestorage.prop.prop_t'
Set-PropertyLine $spiceStorageTool 'spaceToolRechargeRate' $(if ($config.RestoreVanillaSpiceStorageCooldown) {
    "float spaceToolRechargeRate $($config.Values.SpiceStorageCooldownSeconds)"
} else { 'float spaceToolRechargeRate 10' })

$colonyToolCooldowns = @(
    @('placehappinessbooster', 'RestoreVanillaHappinessBoosterCooldown', 'HappinessBoosterCooldownSeconds'),
    @('placeloyaltybooster', 'RestoreVanillaLoyaltyBoosterCooldown', 'LoyaltyBoosterCooldownSeconds'),
    @('placeuberturret', 'RestoreVanillaUberTurretCooldown', 'UberTurretCooldownSeconds'),
    @('placeembassy', 'RestoreVanillaEmbassyCooldown', 'EmbassyCooldownSeconds')
)
foreach ($tool in $colonyToolCooldowns) {
    $toolPath = Join-Path $project "spacetools~\$($tool[0]).prop.prop_t"
    $cooldown = if ($config[$tool[1]]) { $config.Values[$tool[2]] } else { 10 }
    Set-PropertyLine $toolPath 'spaceToolRechargeRate' "float spaceToolRechargeRate $cooldown"
}

$enabledFlags = @($knownOptions | Where-Object { $_ -ne 'Values' -and $config[$_] }) -join ','
if (!$enabledFlags) { $enabledFlags = 'none' }
$config | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $reports 'build-config.json') -Encoding UTF8
Push-Location $ToolDirectory
try {
    # SporeModder's registries are resolved relative to its working directory.
    # Do not change compression settings without rerunning package verification.
    # Windows PowerShell otherwise treats Java's harmless preferences warning
    # on stderr as a terminating error. Check each native exit code explicitly.
    $ErrorActionPreference = 'Continue'
    try {
        if ($config.InstantPlanetaryDialogue) {
            $uiLog = @(& java --class-path $jar (Join-Path $PSScriptRoot 'tools\PatchCommScreen.java') $commReference $commProject 2>&1)
        } else {
            if (Test-Path -LiteralPath $commProject) { Remove-Item -LiteralPath $commProject -Force }
            $uiLog = @('Instant planetary dialogue disabled; communication override omitted.')
            $LASTEXITCODE = 0
        }
        $uiExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = 'Stop' }
    $uiLog | Set-Content -LiteralPath (Join-Path $reports 'ui-patch.log') -Encoding UTF8
    if ($uiExit -ne 0) { throw 'Communication UI generation failed. See reports/ui-patch.log.' }
    $ErrorActionPreference = 'Continue'
    try {
        $packLog = @(& $smfx pack $project $packageCandidate 2>&1)
        $packExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = 'Stop' }
    $packLog | Set-Content -LiteralPath (Join-Path $reports 'pack.log') -Encoding UTF8
    if ($packExit -ne 0 -or !(Test-Path -LiteralPath $packageCandidate)) {
        throw 'SporeModder FX packing failed. See reports/pack.log.'
    }
    $ErrorActionPreference = 'Continue'
    try {
        $verifyLog = @(& java --class-path $jar (Join-Path $PSScriptRoot 'tools\VerifyPackage.java') $kisu $packageCandidate (Join-Path $PSScriptRoot 'reference\vanilla\PatchData.package') $commProject $enabledFlags $project 2>&1)
        $verifyExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = 'Stop' }
    $verifyLog | Set-Content -LiteralPath (Join-Path $reports 'verification.txt') -Encoding UTF8
    if ($verifyExit -ne 0) {
        $verifyLog | Write-Output
        throw 'Package verification failed; installer archive was not produced.'
    }
    Copy-Item -LiteralPath $packageCandidate -Destination $package -Force
    # build-runtime.ps1 adds the DLL and manifest to the installable archive.
    # Do not emit a misleading package-only .sporemod from the runtime edition.
    Get-Sha256FileRecord -Path $package |
        Select-Object Hash, Path | Format-List |
        Out-String | Set-Content -LiteralPath (Join-Path $reports 'SHA256.txt') -Encoding UTF8
    $verifyLog | Write-Output
    Write-Output "Built $package"
} finally {
    Pop-Location
    if ((Test-Path -LiteralPath $stageRoot) -and $stageRoot.StartsWith([System.IO.Path]::GetTempPath(), [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $stageRoot -Recurse -Force
    }
}
