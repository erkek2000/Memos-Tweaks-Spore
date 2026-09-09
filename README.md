# ERKEK2000 QoL Runtime 0.4.0

This is the separate DLL-enabled edition. It was copied from the package-only
ERKEK2000 QoL 0.3.0 project and keeps that edition's package changes and
build-time configuration. Do not install both editions together.

A package-and-DLL personal fork of **KisuTweaks 1.5**, built from the
local Kisu package using **SporeModder FX 2.2.27**. Kisu is the author of the
inherited overhaul, assets, extra tools, and white spice. This project applies
the differences requested in [mod-features.md](mod-features.md).

## Files to use

- `dist/ERKEK2000_QoL_Runtime.sporemod`: installable package containing both
  the data package and runtime DLL.
- [IMPLEMENTED.md](IMPLEMENTED.md): exact changes and retained behavior.
- [ON-HOLD.md](ON-HOLD.md): unimplemented features and discussion points.
- [TESTING.md](TESTING.md): verification results and in-game checks.
- [config.psd1](config.psd1): build-time switches for every implemented
  ERKEK-specific difference from Kisu.

**Not installed and not tested in-game.** This edition requires the Spore
ModAPI Launcher Kit and must be launched through its launcher.

## Manual installation, when ready to test

1. Close Spore and back up the entire `%APPDATA%\Spore` folder.
2. Move conflicting `KisuTweaks.package`, `NambuCargoStack999.package`, or either
   ERKEK2000 QoL edition out of the game's loading folders to a backup directory.
   This mod replaces Kisu; do not install both.
3. Install `dist/ERKEK2000_QoL_Runtime.sporemod` with the ModAPI Easy Installer.
4. Launch Galactic Adventures through the ModAPI Launcher.

Do not manually copy only the data package: the Bio Protector behavior requires
the matching DLL. No installer script here touches saves or the game executable.

Runtime 0.3.0 also adds guarded ESC/TAB communication closing, automatic spice
pickup when the ship reaches a player-owned colony system in the galaxy map,
live enforcement of the configured cargo stack limit (999 by default), and
colony-planner shortcuts: **1** House, **2** Entertainment, **3** Factory,
**4** Turret. The number keys activate the matching visible planner item using
its normal click path; they do not buy or place a building automatically. See
`TESTING.md`; these DLL behaviors are statically verified but have not yet been
play-tested.

Runtime 0.3.1 adds a checked build-time setting for the maximum number of trade
routes. The distributed build retains Kisu's limit of 10; enabling
`RestoreVanillaMaximumTradeRoutes` uses the configurable
`Values.MaximumTradeRoutes` value (5 by default).

Runtime 0.3.2 adds the equivalent setting for the amount of spice bought in one
transaction. The distributed build retains Kisu's cap of 999; enabling
`RestoreVanillaMaximumSpiceBought` uses `Values.MaximumSpiceBought` (200 by
default).

Runtime 0.3.3 adds a setting for passive spice storage per colony. The release
retains Kisu's capacity of 15; enabling `RestoreVanillaColonySpiceStorage` uses
`Values.ColonySpiceStorage` (5 by default).

Runtime 0.4.0 makes the existing Space-stage tool hotbar respond to unmodified
number keys in every tool tab. **1** through **9**, and **0** where a native
panel defines a tenth slot, are delivered to the currently visible native tool
panel rather than a separate custom bar. The game's own panel remains
responsible for slot ordering, availability, cooldowns, selection, and cursor
behavior. Colony-planner **1** through **4** shortcuts retain priority.

## Existing galaxies

This build inherits Kisu's galaxy-generation changes, extra cargo upgrades,
white spice, and 999 cargo tuning. Existing saves do not necessarily adopt
all of these changes. This runtime build now attempts to repair the old save's
99 limit by applying the configured limit through the live inventory API. A
fresh test galaxy is useful, but successful 999 collection/display/selling and
save/reload behavior must be demonstrated before treating it as solved.

Do not reset your current galaxy merely to install this build. Preserve it and
use a separate, disposable test galaxy when ready. Changing a package does not
regenerate existing stars, reverse earned badges, or refund earlier purchases.

## Editing and rebuilding

`project/` is the editable SporeModder FX source. Add it as an external project
in SporeModder FX to inspect/edit it. Properties use `.prop.prop_t`; the model,
icons, locale, and package priority signature retain their original formats.

The checked full build command, from this directory, is:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-runtime.ps1
```

Edit `config.psd1` before rebuilding to enable or disable individual changes
and to customize their numeric values.
The default file produces the standard runtime edition. For example, the
following command builds the included all-Kisu verification preset while still
including the runtime DLL:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-runtime.ps1 `
  -ConfigurationFile .\configs\KisuBaseline.psd1
```

Configuration is applied while building; Spore does not read `config.psd1` and
an already-built package does not change when the file is edited. Each build
records its effective settings in `reports/build-config.json`. The build uses a
temporary staged project and does not rewrite the editable `project/` source.

This uses the existing sibling `../Tools/SporeModderFX` installation and Java
21. It packs the source with SMFX, verifies the compiled package against Kisu
and the decoded vanilla references, builds the x86 DLL, then creates the
three-file `.sporemod` archive.
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
