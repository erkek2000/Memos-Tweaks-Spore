param(
    [string]$ConfigurationFile = (Join-Path $PSScriptRoot 'config.psd1'),
    [string]$SdkDirectory = (Join-Path $PSScriptRoot '..\..\Tools\Spore-ModAPI-SDK'),
    [string]$MSBuildPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
)
$ErrorActionPreference = 'Stop'

# Some launchers supply both Path and PATH in the inherited environment.
# .NET Framework MSBuild treats those case-insensitive names as duplicate
# dictionary keys and then cannot start CL.exe. Recreate one canonical entry
# in this build process without changing the user or machine environment.
$processPath = [Environment]::GetEnvironmentVariable(
    'Path', [EnvironmentVariableTarget]::Process)
[Environment]::SetEnvironmentVariable(
    'PATH', $null, [EnvironmentVariableTarget]::Process)
[Environment]::SetEnvironmentVariable(
    'Path', $processPath, [EnvironmentVariableTarget]::Process)

$SdkDirectory = (Resolve-Path -LiteralPath $SdkDirectory).Path
$MSBuildPath = (Resolve-Path -LiteralPath $MSBuildPath).Path
$baseLibrary = Join-Path $SdkDirectory 'lib\Release\SporeModAPIBase.lib'
$importLibrary = Join-Path $SdkDirectory 'dll\Release\SporeModAPI.lib'
if (!(Test-Path -LiteralPath $baseLibrary) -or !(Test-Path -LiteralPath $importLibrary)) {
    throw 'The sparse ModAPI SDK libraries have not been built. See runtime/README.md.'
}

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'build.ps1') `
    -ConfigurationFile $ConfigurationFile
if ($LASTEXITCODE -ne 0) { throw 'Data-package build failed.' }

$runtimeConfig = Import-PowerShellDataFile -LiteralPath $ConfigurationFile
function ConvertTo-CompilerBoolean([bool]$Value) {
    if ($Value) { return 1 }
    return 0
}
$preventBioDisasters = ConvertTo-CompilerBoolean $runtimeConfig.PreventBioDisastersWithBioProtector
$closeWithEscape = ConvertTo-CompilerBoolean $runtimeConfig.CloseDialogueWithEscape
$closeWithTab = ConvertTo-CompilerBoolean $runtimeConfig.CloseDialogueWithTab
$fastDialogueOpening = ConvertTo-CompilerBoolean $runtimeConfig.FastDialogueOpening
$collectGalaxySpice = ConvertTo-CompilerBoolean $runtimeConfig.CollectSpiceAtGalaxyStars
$enforceCargoStackLimit = ConvertTo-CompilerBoolean $runtimeConfig.EnforceCargoStackLimit
$buildingShortcuts = ConvertTo-CompilerBoolean $runtimeConfig.BuildingKeyboardShortcuts
$spaceHotbarShortcuts = ConvertTo-CompilerBoolean $runtimeConfig.SpaceHotbarKeyboardShortcuts
$colonyPatternButtons = ConvertTo-CompilerBoolean $runtimeConfig.ColonyPatternButtons
$cropCircleUplift = ConvertTo-CompilerBoolean $runtimeConfig.CropCircleUplift
$cargoStackLimit = [int]$runtimeConfig.Values.CargoStackLimit
$houseCost = [int]$runtimeConfig.Values.HouseCost
$entertainmentCost = [int]$runtimeConfig.Values.EntertainmentCost
$factoryCost = [int]$runtimeConfig.Values.FactoryCost
$turretCost = [int]$runtimeConfig.Values.TurretCost
$cropCircleUpliftIntervalSeconds = [int]$runtimeConfig.Values.CropCircleUpliftIntervalSeconds

$runtimeProject = Join-Path $PSScriptRoot 'runtime\ERKEK2000_QoL_Runtime.vcxproj'
$buildLog = Join-Path $PSScriptRoot 'reports\runtime-build.log'
# TrackFileAccess=false: this project always builds with /t:Rebuild, so MSBuild
# dependency tracking adds nothing. It is disabled because the tracking layer
# (Microsoft.Build.Utilities.FileTracker) crashes on profiles where
# CSIDL_COMMON_APPDATA resolves to an empty path (e.g. sandboxed shells).
& $MSBuildPath $runtimeProject /t:Rebuild /p:Configuration=Release /p:Platform=Win32 `
    /p:TrackFileAccess=false `
    /p:RuntimePreventBioDisasters=$preventBioDisasters `
    /p:RuntimeCloseWithEscape=$closeWithEscape `
    /p:RuntimeCloseWithTab=$closeWithTab `
    /p:RuntimeFastDialogueOpening=$fastDialogueOpening `
    /p:RuntimeCollectGalaxySpice=$collectGalaxySpice `
    /p:RuntimeEnforceCargoStackLimit=$enforceCargoStackLimit `
    /p:RuntimeBuildingShortcuts=$buildingShortcuts `
    /p:RuntimeSpaceHotbarShortcuts=$spaceHotbarShortcuts `
    /p:RuntimeColonyPatternButtons=$colonyPatternButtons `
    /p:RuntimeCropCircleUplift=$cropCircleUplift `
    /p:RuntimeHouseCost=$houseCost `
    /p:RuntimeEntertainmentCost=$entertainmentCost `
    /p:RuntimeFactoryCost=$factoryCost `
    /p:RuntimeTurretCost=$turretCost `
    /p:RuntimeCargoStackLimit=$cargoStackLimit `
    /p:RuntimeCropCircleUpliftIntervalSeconds=$cropCircleUpliftIntervalSeconds /m 2>&1 |
    Tee-Object -FilePath $buildLog
if ($LASTEXITCODE -ne 0) { throw 'Runtime DLL build failed. See reports/runtime-build.log.' }

$package = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.package'
$dll = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.dll'
$manifest = Join-Path $PSScriptRoot 'ModInfo.xml'
$archive = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.sporemod'
$packageOnlyArchive = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime_PackageOnly.sporemod'
foreach ($path in @($package, $dll, $manifest)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing archive input: $path" }
}

Add-Type -AssemblyName System.IO.Compression
$stream = [System.IO.File]::Open($archive, [System.IO.FileMode]::Create)
try {
    $zip = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($inputPath in @($package, $dll, $manifest)) {
            $entry = $zip.CreateEntry([System.IO.Path]::GetFileName($inputPath))
            $entryStream = $entry.Open()
            $inputStream = [System.IO.File]::OpenRead($inputPath)
            try { $inputStream.CopyTo($entryStream) }
            finally { $inputStream.Dispose(); $entryStream.Dispose() }
        }
    } finally { $zip.Dispose() }
} finally { $stream.Dispose() }

# Keep a true package-only control beside the runtime archive. It is generated
# from the exact same package, but its manifest has no DLL prerequisite, so an
# in-game crash can be assigned to package data or runtime code unambiguously.
$packageOnlyManifest = [xml](Get-Content -LiteralPath $manifest -Raw)
foreach ($node in @($packageOnlyManifest.SelectNodes('//prerequisite[not(@game)]'))) {
    [void]$node.ParentNode.RemoveChild($node)
}
$stream = [System.IO.File]::Open($packageOnlyArchive, [System.IO.FileMode]::Create)
try {
    $zip = New-Object System.IO.Compression.ZipArchive($stream, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $packageEntry = $zip.CreateEntry([System.IO.Path]::GetFileName($package))
        $entryStream = $packageEntry.Open()
        $inputStream = [System.IO.File]::OpenRead($package)
        try { $inputStream.CopyTo($entryStream) }
        finally { $inputStream.Dispose(); $entryStream.Dispose() }

        $manifestEntry = $zip.CreateEntry([System.IO.Path]::GetFileName($manifest))
        $writer = New-Object System.IO.StreamWriter($manifestEntry.Open(), (New-Object System.Text.UTF8Encoding($false)))
        try { $writer.Write($packageOnlyManifest.OuterXml) }
        finally { $writer.Dispose() }
    } finally { $zip.Dispose() }
} finally { $stream.Dispose() }

Get-FileHash -Algorithm SHA256 -LiteralPath $package, $dll, $archive, $packageOnlyArchive |
    Select-Object Hash, Path | Format-List | Out-String |
    Set-Content -LiteralPath (Join-Path $PSScriptRoot 'reports\SHA256.txt') -Encoding UTF8

Write-Output "Built $package"
Write-Output "Built $dll"
Write-Output "Built $archive"
Write-Output "Built $packageOnlyArchive"
