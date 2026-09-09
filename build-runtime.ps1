param(
    [string]$ConfigurationFile = (Join-Path $PSScriptRoot 'config.psd1'),
    [string]$SdkDirectory = (Join-Path $PSScriptRoot '..\Tools\Spore-ModAPI-SDK'),
    [string]$MSBuildPath = 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
)
$ErrorActionPreference = 'Stop'

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
$collectGalaxySpice = ConvertTo-CompilerBoolean $runtimeConfig.CollectSpiceAtGalaxyStars
$enforceCargoStackLimit = ConvertTo-CompilerBoolean $runtimeConfig.EnforceCargoStackLimit
$buildingShortcuts = ConvertTo-CompilerBoolean $runtimeConfig.BuildingKeyboardShortcuts
$spaceHotbarShortcuts = ConvertTo-CompilerBoolean $runtimeConfig.SpaceHotbarKeyboardShortcuts
$cargoStackLimit = [int]$runtimeConfig.Values.CargoStackLimit

$runtimeProject = Join-Path $PSScriptRoot 'runtime\ERKEK2000_QoL_Runtime.vcxproj'
$buildLog = Join-Path $PSScriptRoot 'reports\runtime-build.log'
& $MSBuildPath $runtimeProject /t:Rebuild /p:Configuration=Release /p:Platform=Win32 `
    /p:RuntimePreventBioDisasters=$preventBioDisasters `
    /p:RuntimeCloseWithEscape=$closeWithEscape `
    /p:RuntimeCloseWithTab=$closeWithTab `
    /p:RuntimeCollectGalaxySpice=$collectGalaxySpice `
    /p:RuntimeEnforceCargoStackLimit=$enforceCargoStackLimit `
    /p:RuntimeBuildingShortcuts=$buildingShortcuts `
    /p:RuntimeSpaceHotbarShortcuts=$spaceHotbarShortcuts `
    /p:RuntimeCargoStackLimit=$cargoStackLimit /m 2>&1 |
    Tee-Object -FilePath $buildLog
if ($LASTEXITCODE -ne 0) { throw 'Runtime DLL build failed. See reports/runtime-build.log.' }

$package = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.package'
$dll = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.dll'
$manifest = Join-Path $PSScriptRoot 'ModInfo.xml'
$archive = Join-Path $PSScriptRoot 'dist\ERKEK2000_QoL_Runtime.sporemod'
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

Get-FileHash -Algorithm SHA256 -LiteralPath $package, $dll, $archive |
    Select-Object Hash, Path | Format-List | Out-String |
    Set-Content -LiteralPath (Join-Path $PSScriptRoot 'reports\SHA256.txt') -Encoding UTF8

Write-Output "Built $package"
Write-Output "Built $dll"
Write-Output "Built $archive"
