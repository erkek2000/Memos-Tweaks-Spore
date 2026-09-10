# ERKEK2000 QoL Runtime 0.5.8

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
- [REVERSE_ENGINEERING.md](REVERSE_ENGINEERING.md): reusable SDK, package,
  Ghidra, planner, persistence, and crash-analysis findings.
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

Runtime 0.4.1 adds two complete package-side gameplay switches. Enabling
`RestoreVanillaHomeworldSpiceProduction` changes the homeworld multiplier from
Kisu's 0.25 to `Values.HomeworldSpiceProductionMultiplier` (0.025 by default).
Enabling `RestoreVanillaSpiceStorageCooldown` changes the Spice Storage tool's
recharge from Kisu's 10 seconds to `Values.SpiceStorageCooldownSeconds` (30 by
default). Both switches retain Kisu's behavior in the distributed release.

Runtime 0.4.2 adds independent equivalents for the Happiness Booster, Loyalty
Booster, Uber Turret, and Embassy. Each can retain Kisu's 10-second recharge or
use its own configured cooldown (30 seconds by default, matching vanilla).

Runtime 0.5.0 adds three colony-planner buttons: **Copy colony pattern**,
**Apply saved pattern**, and **Apply to all colonies**. Copying stores every
building, turret, civic decoration, and empty slot from the current colony for
the rest of the game session and enables the two apply buttons. Applying
replaces the corresponding target slots. It spends only available Sporebucks,
never makes the balance negative, leaves unaffordable occupied slots unchanged,
and displays **Build incomplete: funds ran out.** after a partial build.
Hovering either apply button shows the total required Sporebucks in green when
affordable or red when unaffordable, together with the partial-build warning.
The all-colonies action operates on the player colonies loaded for the current
planet. Runtime 0.5.1 also saves the latest copy to
`%APPDATA%\Spore\ERKEK2000_QoL\colony-pattern.bin` and automatically reloads
it after restarting Spore, including in another galaxy. Saving uses an atomic
replacement; loading rejects incompatible versions, bad checksums, excessive
slot counts, unsupported noun types, and invalid numeric data. This direct
runtime editor integration requires the backed-up-save test in `TESTING.md`
before normal play.

Runtime 0.5.2 attempted to correct the first loading crash by delaying the
planner buttons, but the same crash recurred. Runtime 0.5.3 broadens the
lifecycle isolation: no custom Space-only input procedure is attached to the
root UI during Civilization loading. The planner controls are attached only to
their own buttons after the Space game and colony planner are active, and all
Space input procedures detach when leaving Space stage. That build still
crashed identically; its minidump symbolizes the crash to the window-procedure
list walker inside `UTFWin::Window::AddWinProc` while the game rebuilds the UI
tree at the end of a planet load. Runtime 0.5.4 therefore attaches each
Space-only procedure exactly once, only when the Space game is fully entered
and not loading, and never detaches it again during the session. See
`TESTING.md` for the regression test and the NoUI crash-isolation variant.

Runtime 0.5.5 disables the generated instant-planetary-dialogue screen
override. The package-only control reproduced the loading-screen crash, and
the crash site is UTFWin's window-procedure setup; the native communication
screen is therefore retained until that override can be proven safe.

Runtime 0.5.6 restores instant planetary dialogue safely through the DLL. It
waits for the original native communication screen to exist, then sets only
its two opening Glide effects to zero duration and zero offset. No `CommScreen`
resource is replaced, no custom window is created, and dialogue content and
all native buttons remain unchanged. `FastDialogueOpening` controls this
runtime feature and defaults on.

Runtime 0.5.6 gives Crop Circles a persistent, non-stacking uplift effect.
On a non-homeworld Creature, Tribe, or Civilization planet, the first
successful ground or water hit starts one 20-minute step. Creature advances to
Tribe, Tribe advances to Civilization, and Civilization advances to a native
Empire after a third step. The interval is ten times the
native Monolith mean evolution interval (the requested 90% slower rate).
Repeated Crop Circle hits do not shorten the wait; Monolith is unchanged.
Progress is stored in `%APPDATA%\Spore\ERKEK2000_QoL\crop-circle-uplift.bin`,
so elapsed real time survives closing and reopening the game.

Runtime 0.5.8 fixes the crash produced by the 0.5.0 apply actions. The city
hall derives from `cBuilding`, so the in-memory copy recorded it as an ordinary
building and applying it destroyed the hall and corrupted the colony. Copying
now records the city hall - and any other unsupported noun - as a protected
slot, and applying never creates or removes a protected slot or a target city
hall. A `colony-pattern.bin` written before this fix contains the hall as a
building and is still rejected by the strict loader, so copy the pattern again
after updating. The crash analysis is in `REVERSE_ENGINEERING.md`.

Runtime 0.5.9 corrects pasted orientation and hardens the apply actions.
Orientations are now captured relative to the planet surface frame at each
slot (`PlanetModel.GetOrientation`) instead of the layout quaternion, so a
pattern pasted into a colony elsewhere on the planet stays upright and faces
the target colony's direction. Applying re-asserts position, scale, and
orientation after the game's own add/set calls, which may reposition or
reorient objects. Applying never destroys a target object whose noun the
capture step cannot recreate (the hall and any special colony structure are
left in place and their slots skipped), the all-colonies action only touches
colonies on the same planet as the open planner colony and processes that
colony last, and the pattern format version is bumped to 2, so a pattern
copied before 0.5.9 is rejected and must be copied again. Copying now reports
the copied counts (`Copied N buildings, M decorations, K turrets.`) and warns
when the city containers hold objects that are not present in any layout slot
- those objects cannot be captured. Each copy
and apply also writes a crash-safe diagnostic log to
`%APPDATA%\Spore\ERKEK2000_QoL\colony-copy.log` and `colony-apply.log`; those
files pinpoint the exact slot if a crash still occurs.

Runtime 0.5.10 responds to the first in-game tests of 0.5.9. **Automatically
speeding up the communication screen's opening is disabled by default**
(`FastDialogueOpening`); a 0.5.9 session crashed inside the game's effect code
when that feature walked the comm panel's window-procedure list, so the
cosmetic speed-up is compiled out of the standard build until it can be proven
safe (the code remains, guarded, for a `FastDialogueOpening = $true`
experimental build). Applying a pattern also no longer destroys and recreates a
target object that already has the same noun: the first such replacement (an
ornament) crashed the game's noun factory, so matching slots are now updated in
place and the destroy/create path is used only when the noun must change. See
`REVERSE_ENGINEERING.md` for both crash analyses.

Runtime 0.5.11 makes the communication opening instant again. Every game call in
the panel walk (`GetNextWinProc`, the `IGlideEffect` cast, `SetTime`,
`SetOffset`, `Revalidate`) is now individually fault-guarded, so a call that is
unsafe in a transient screen state is skipped instead of crashing, and the
attempt is retried for up to three seconds after the screen appears so a later,
stable attempt still applies the speed-up. A `comm-open.log` next to the colony
pattern records how many effects were made instant and how many calls faulted,
so the result is verifiable in-game. `FastDialogueOpening` defaults on again.

Runtime 0.5.12 fixes the reason 0.5.11 still only logged faults. The Ghidra
type database shows the game's `IGlideEffect` vtable is seven entries with
`SetOffset` at `0x18`, and `IBiStateEffect` has `SetTime` at `0x18`, but the SDK
models `IGlideEffect` as deriving from `IBiStateEffect`; the compiler therefore
called `SetTime` at `0x18` (the game's `SetOffset`, hence the original null
read) and `SetOffset` at `0x40` (past the vtable). `SetTime` is now called
through `IBiStateEffect` and `SetOffset` through a mirror of the game's
IGlideEffect vtable, so both reach the correct functions.

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

This uses the workspace's `../../Tools/SporeModderFX` installation and Java
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
