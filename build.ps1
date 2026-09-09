param(
    [string]$ToolDirectory = (Join-Path $PSScriptRoot '..\Tools\SporeModderFX'),
    [string]$ConfigurationFile = (Join-Path $PSScriptRoot 'config.psd1')
)
$ErrorActionPreference = 'Stop'
$ToolDirectory = (Resolve-Path -LiteralPath $ToolDirectory).Path
$sourceProject = Join-Path $PSScriptRoot 'project'
$dist = Join-Path $PSScriptRoot 'dist'
$reports = Join-Path $PSScriptRoot 'reports'
$package = Join-Path $dist 'ERKEK2000_QoL.package'
$smfx = Join-Path $ToolDirectory 'smfx.exe'
$jar = Join-Path $ToolDirectory 'SporeModderFX.jar'
$kisu = Join-Path $PSScriptRoot '..\KisuTweaks-1.5\KisuTweaks-1.5\KisuTweaks.package'
$commReference = Join-Path $PSScriptRoot 'reference\vanilla\PatchData-CommScreen-3.spui'
$kisuReference = Join-Path $PSScriptRoot 'reference\kisu'
$ConfigurationFile = (Resolve-Path -LiteralPath $ConfigurationFile).Path
$config = Import-PowerShellDataFile -LiteralPath $ConfigurationFile
$knownOptions = @(
    'RestoreVanillaBadgeRequirements',
    'RestoreGroxExclusiveRadius',
    'RestoreGroxSpreadRadius',
    'RestoreCoreTravelRestriction',
    'RestoreGeneralSpiceProduction',
    'RestoreColonyBuildingCosts',
    'RestoreShieldCooldown',
    'InstantPlanetaryDialogue',
    'Values'
)
foreach ($option in $knownOptions | Where-Object { $_ -ne 'Values' }) {
    if (!$config.ContainsKey($option) -or $config[$option] -isnot [bool]) {
        throw "Configuration option '$option' must be present and Boolean."
    }
}
$knownValues = @(
    'GroxExclusiveRadius', 'GroxSpreadRadius', 'CoreTravelMinimum',
    'CoreTravelMaximum', 'SpiceProductionMultiplier', 'HouseCost',
    'EntertainmentCost', 'FactoryCost', 'TurretCost', 'ShieldCooldownSeconds'
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
    # A .sporemod is a ZIP containing the package and its installer manifest.
    Add-Type -AssemblyName System.IO.Compression
    $archivePath = Join-Path $dist 'ERKEK2000_QoL.sporemod'
    $stream = [System.IO.File]::Open($archivePath, [System.IO.FileMode]::Create)
    try {
        $zip = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            foreach ($inputPath in @($package, (Join-Path $PSScriptRoot 'ModInfo.xml'))) {
                $entry = $zip.CreateEntry([System.IO.Path]::GetFileName($inputPath))
                $entryStream = $entry.Open()
                $inputStream = [System.IO.File]::OpenRead($inputPath)
                try { $inputStream.CopyTo($entryStream) }
                finally { $inputStream.Dispose(); $entryStream.Dispose() }
            }
        } finally { $zip.Dispose() }
    } finally { $stream.Dispose() }
    Get-FileHash -Algorithm SHA256 -LiteralPath $package, $archivePath |
        Select-Object Hash, Path | Format-List |
        Out-String | Set-Content -LiteralPath (Join-Path $reports 'SHA256.txt') -Encoding UTF8
    $verifyLog | Write-Output
    Write-Output "Built $package"
    Write-Output "Built $archivePath"
} finally {
    Pop-Location
    if ((Test-Path -LiteralPath $stageRoot) -and $stageRoot.StartsWith([System.IO.Path]::GetTempPath(), [System.StringComparison]::OrdinalIgnoreCase)) {
        Remove-Item -LiteralPath $stageRoot -Recurse -Force
    }
}
