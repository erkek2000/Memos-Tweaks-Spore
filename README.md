# ERKEK2000 QoL 0.3.0

A standalone, package-only personal fork of **KisuTweaks 1.5**, built from the
local Kisu package using **SporeModder FX 2.2.27**. Kisu is the author of the
inherited overhaul, assets, extra tools, and white spice. This project applies
the differences requested in [mod-features.md](mod-features.md).

## Files to use

- [dist/ERKEK2000_QoL.package](dist/ERKEK2000_QoL.package): manually installable mod.
- [dist/ERKEK2000_QoL.sporemod](dist/ERKEK2000_QoL.sporemod): the same package inside
  an optional Easy Installer archive.
- [IMPLEMENTED.md](IMPLEMENTED.md): exact changes and retained behavior.
- [ON-HOLD.md](ON-HOLD.md): unimplemented features and discussion points.
- [TESTING.md](TESTING.md): verification results and in-game checks.
- [config.psd1](config.psd1): build-time switches for every implemented
  ERKEK-specific difference from Kisu.

**Built, but not installed by this task and not tested in-game.** This version
does not need a DLL, DLL injection, or the ModAPI Launcher. It can be used with
Steam's Galactic Adventures executable. The optional installer is only a way
to copy the package; it is not needed for manual installation.

## Manual installation, when ready to test

1. Close Spore and back up the entire `%APPDATA%\Spore` folder.
2. Move conflicting `KisuTweaks.package`, `NambuCargoStack999.package`, or an older
   `ERKEK2000_QoL.package` out of the game's loading folders to a backup directory.
   This mod replaces Kisu; do not install both.
3. Copy **only** `dist/ERKEK2000_QoL.package` to the Galactic Adventures data folder:
   `C:\Program Files (x86)\Steam\steamapps\common\spore 24720\DataEP1`.
4. Launch Galactic Adventures through Steam as usual.

The raw package and installer archive are alternatives; do not copy the whole
project or `.sporemod` archive into DataEP1. No installer script here touches
your saves, game executable, or currently installed mod.

## Existing galaxies

This build inherits Kisu's galaxy-generation changes, extra cargo upgrades,
white spice, and 999 cargo tuning. Existing saves do not necessarily adopt
all of these changes. In particular, your old save previously displayed 99 and
sold down to 98 despite the 999 property; **this build does not fix that bug**.
A fresh test galaxy is useful, but successful 999 collection/display/selling
and save/reload behavior must be demonstrated before treating it as solved.

Do not reset your current galaxy merely to install this build. Preserve it and
use a separate, disposable test galaxy when ready. Changing a package does not
regenerate existing stars, reverse earned badges, or refund earlier purchases.

## Editing and rebuilding

`project/` is the editable SporeModder FX source. Add it as an external project
in SporeModder FX to inspect/edit it. Properties use `.prop.prop_t`; the model,
icons, locale, and package priority signature retain their original formats.

The checked build command, from this directory, is:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

Edit `config.psd1` before rebuilding to enable or disable individual changes
and to customize their numeric values.
The default file produces the standard ERKEK2000 QoL package. For example, the
following command builds the included all-Kisu verification preset:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 `
  -ConfigurationFile .\configs\KisuBaseline.psd1
```

Configuration is applied while building; Spore does not read `config.psd1` and
an already-built package does not change when the file is edited. Each build
records its effective settings in `reports/build-config.json`. The build uses a
temporary staged project and does not rewrite the editable `project/` source.

This uses the existing sibling `../Tools/SporeModderFX` installation and Java
21. It packs the source with SMFX, verifies the compiled package against Kisu
and the decoded vanilla references, then creates the `.sporemod` archive.
It overwrites this project's generated `dist/` artifacts and reports only.
The execution-policy option applies to that PowerShell process, not a permanent
machine setting. Java may emit a preferences/registry warning in the sandbox;
the build checks native exit codes and package contents independently.

The verification script intentionally enforces this release's exact changes.
When adding a new feature, update its expected deltas as well as the source.
There is no live reload or in-game settings screen.

`reference/kisu/` is an untouched decoded copy of the original package;
`reference/vanilla/` contains selected properties extracted from the installed
game for comparison. Neither is packed. The input feature documents are kept
unchanged; `mod-features.md` is authoritative over `mod-features-old.md`.

See [reports/verification.txt](reports/verification.txt),
[reports/SHA256.txt](reports/SHA256.txt), and [PROVENANCE.md](PROVENANCE.md).
