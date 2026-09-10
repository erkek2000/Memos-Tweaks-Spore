# Implemented differences from Kisu 1.5

## Runtime-edition addition

The DLL intercepts creation of the `biospherecollapse` event. It checks the
selected planet's persisted planet-object records for the Bio Protector model
and rejects creation when found. This prevents future eco-disasters on those
planets. Existing active disasters are not cancelled because the public SDK
does not expose a safe removal operation.

The runtime DLL also maps both **ESC** and **TAB** to the communication
screen's normal Goodbye button. It acts only while the communication manager
reports an active screen and that button is visible and enabled; it does not
bypass trading or mission submenu confirmation flows. Static compilation has
passed, but the root-window keyboard dispatch and synthetic click still need
the in-game checks listed in `TESTING.md`.

When the player ship reaches a star while in galaxy-map context, the runtime
collector transfers stored spice from player-controlled colonies in that
system. It runs once per arrival, supports multiple spice planets, stops at
each stack's configured maximum and at the available cargo-slot limit, and
subtracts only the amount the inventory demonstrably accepted. Uncollected
overflow remains stored on its planet.

While the Space-stage colony planner is active, number keys select its visible
building palette items through the same button-click message used by the UI:
**1** House, **2** Entertainment, **3** Factory, and **4** Turret. The handler
does nothing outside the active community editor, ignores modified number keys,
and does not buy or place anything automatically. The SDK exposes the planner
palette and item windows even though the underlying `cCommunityEditor` remains
undocumented. Actual planner focus and selection behavior still require the
in-game check in `TESTING.md`.

Runtime 0.4.0 activates the existing per-tab Space tool hotbar with number
keys. A guarded root input procedure finds visible `SpaceToolPanelUI` instances
and gives their original handler unmodified **0** through **9** key-down
messages. This uses the panel's native slot order and selection logic instead
of introducing a second bar or assignment format. It is inactive in the
colony planner, where the dedicated building shortcuts retain priority, and
requires the in-game matrix in `TESTING.md`.

The runtime also reapplies the configured cargo stack limit to the live player
inventory whenever Space stage is active. The default is **999**. This is
intended to repair an old save whose persisted inventory limit remained 99 even
though the package property was 999; it uses the game's own
`SetMaxCargoAmount()` path and does not edit save files directly. Compilation
and configuration tests pass, but display, transaction, and save persistence
still require the old-save play test in `TESTING.md`.

Specification: `mod-features.md`. All values below were checked against the
local vanilla game properties, not inferred solely from Kisu's README.

| Request | Kisu value | This build | Resource/property |
| --- | --- | --- | --- |
| Vanilla badge requirements | 110 tiered overrides | Overrides omitted; vanilla requirements apply | `space_badges~/*`, except `captain` |
| Restore Grox-exclusive radius | 30 pc | 100 pc | `gametuning~/SpaceSolarSystem`, `grobOnlyRadius` |
| Restore core travel-restriction band | 25-35 pc | 70-100 pc | `gametuning~/SpaceGalacticConstants`, `galacticCoreTravelRadii` |
| Remove 50% general spice production boost | 0.00075 | 0.0005 | `gametuning~/SpaceEconomy`, `spaceEconomySpiceProductionMultiplier` |
| Restore house base cost | 16,000 | 25,600 | `animations~/CityGameBuildingTuning`, `SleepCostSpace` |
| Restore entertainment base cost | 8,000 | 12,800 | Same resource, `EntertainmentCostSpace` |
| Restore factory base cost | 12,000 | 19,200 | Same resource, `IndustryCostSpace` |
| Restore turret base cost | 10,000 | 16,000 | Same resource, `DefenseCostSpace` |
| Restore shield cooldown | 120 seconds | 180 seconds | `spacetools~/shield`, `spaceToolRechargeRate` |
| Instant planetary dialogue popup | 1.2 s / 0.4 s vertical entrance glides | Both main communication glides have zero time and offset | `layouts_atlas~/CommScreen-3.spui` |

The 110 badge files cover all five tiers of 22 badge families, including Kisu's
harder Golden Touch and Merchant changes. "Keep them vanilla" is interpreted
as restoring all tiered requirements, not only those Kisu made easier.
The Captain's badge still grants Kisu's extra seven cargo slots; its unlock
requirement is already vanilla. Removing that reward would remove a cargo
feature that was not requested for removal.

## Related radius choice

This build also restores `grobStarsSpreadRadius` from 35 to **105**, its vanilla
value, alongside the requested 100 pc exclusive radius. This companion change
was raised for clarification and provisionally selected because Kisu changed
both together. It is not separately listed in the user's spec. It affects the
spread of Grox star placement; confirm this preference before a long-term new
galaxy. `grobEmpireSize` stays **300**, as in Kisu, not vanilla's 2400.

## Retained Kisu features

All other compiled gameplay resources are retained unchanged, including:

- 999 cargo tuning, five cargo upgrades, and the Captain's extra cargo reward.
- Ten trade routes, faster transitions, camera/FOV and ship changes.
- White spice, its localization/model, and cargo-upgrade icons.
- Increased spice storage and the separate homeworld production multiplier
  of 0.25. Only the general 50% production boost was requested for removal.
- Increased moon/ring chances and the smaller 300-system Grox empire.
- Kisu's disaster-frequency tuning, trade bundles, consequence powers,
  archetype changes, sculpting tools, and the earlier-stage tweaks.

Kisu's full baseline changelog is in
`../KisuTweaks-1.5/KisuTweaks-1.5/README.md`. Retention means the package data is
preserved; it is not a claim that every inherited feature works on an old save.

The communication-screen change starts from the installed game's vanilla
PatchData resource. It changes only the two vertical Glide processors attached
to control IDs `0x05E4E5F0` and `0x05E4E5F8`: `(1.2 s, 0x400 px)` and
`(0.4 s, 0x800 px)` become `(0 s, 0x0 px)`. Other communication-screen motion,
including button/tooltip motion, is retained. It uses a package UI resource and
does not need a DLL. It may conflict with other mods that replace the same SPUI.

## Build-time configuration

The package foundation inherited from version 0.3.0 adds `config.psd1`. It
independently controls vanilla badge
requirements, the two Grox radii, the core travel band, general spice
production, colony building costs, shield cooldown, and instant planetary
dialogue. Its `Values` section also customizes both Grox radii, both core-band
limits, the spice multiplier, four building costs, and shield cooldown.
Disabled switches restore the corresponding Kisu 1.5 behavior and ignore their
custom value until re-enabled.

Runtime 0.3.0 extends the same build-time file with switches for Bio Protector
immunity, ESC closing, TAB closing, galaxy-map spice collection, and live cargo
stack-limit enforcement, plus colony building shortcuts.
`Values.CargoStackLimit` controls the enforced limit (default 999). The full
build converts those values to compiler definitions and rebuilds the DLL, so
presets cannot accidentally reuse a DLL compiled with older settings.

Runtime 0.3.1 adds `RestoreVanillaMaximumTradeRoutes` and
`Values.MaximumTradeRoutes`. It retains Kisu's limit of 10 by default; enabling
the option uses 5 (patched vanilla) or another positive whole number selected
before rebuilding.

Runtime 0.3.2 adds `RestoreVanillaMaximumSpiceBought` and
`Values.MaximumSpiceBought`. It retains Kisu's per-transaction cap of 999 by
default; enabling the option uses 200 (vanilla) or another positive whole
number selected before rebuilding.

Runtime 0.3.3 adds `RestoreVanillaColonySpiceStorage` and
`Values.ColonySpiceStorage`. It retains Kisu's passive capacity of 15 per
colony by default; enabling the option uses 5 (vanilla) or another positive
whole number selected before rebuilding.

Runtime 0.4.0 adds the `SpaceHotbarKeyboardShortcuts` DLL switch. It is enabled
in the release and disabled in the Kisu-baseline preset.

Runtime 0.4.1 adds `RestoreVanillaHomeworldSpiceProduction` with
`Values.HomeworldSpiceProductionMultiplier`, and
`RestoreVanillaSpiceStorageCooldown` with `Values.SpiceStorageCooldownSeconds`.
They retain Kisu's 0.25 homeworld multiplier and 10-second tool cooldown by
default; enabling them uses vanilla defaults of 0.025 and 30 seconds. The
custom-value build verifies that both configured values reach the package.

Runtime 0.4.2 adds independently configurable vanilla cooldown restoration for
the Happiness Booster, Loyalty Booster, Uber Turret, and Embassy. Their four
switches retain Kisu's 10-second values by default; their four numeric values
default to the verified vanilla recharge time of 30 seconds.

Runtime 0.5.0 adds an in-session colony pattern copier and two apply actions.
The three buttons appear only in the active Space colony planner. A copied
pattern includes buildings, turrets, civic decorations, empty slots, model,
scale, and layout-relative orientation. Applying replaces matching slots in the
current colony or every loaded player colony on the current planet. Empty saved
slots remove their target contents for free; occupied saved slots are processed
in building, decoration, then turret order and are skipped when their full
construction cost is no longer affordable. The balance cannot become negative.
The apply hover text reports the required amount in green or red and warns that
construction stops when funds run out. A partial application displays a visible
`Build incomplete: funds ran out.` warning. `ColonyPatternButtons` controls the
DLL feature at build time and defaults on in the release.

Preset application is intentionally blocked on a homeworld. Its city templates
use layouts that are not safely interchangeable with ordinary colonies; the
feature displays a warning instead of altering the homeworld or risking a
community-editor crash.

Runtime 0.5.8 fixes the apply crash. `CaptureLayout` never validated nouns, so
the city hall (which derives from `cBuilding`) was captured as an ordinary
building; applying then destroyed the hall and corrupted the city. Copying now
records the city hall, and any other unsupported noun, as a new
`PatternKind::Protected`, and apply never creates or removes a protected slot or
a target city hall (with a defensive guard in `RemoveExisting` as well). New
files round-trip the protected slot; files written before the fix still contain
the hall as a building and are rejected by the strict loader, so the pattern
must be copied again.

Runtime 0.5.9 fixes pasted orientation and hardens the apply path after a
fresh crash symbolization. The 20:59:29 apply-all crash faulted in
`CreatePatternObject` reading `pattern.nounID` from a wild pointer - the same
signature as the hall crash - so the remaining manual destruction of objects
the feature cannot recreate was closed symmetrically: apply never destroys a
target object unless its noun is one the capture step recreates, and
`RemoveExisting` enforces the same rule. Orientations are captured and
restored against `PlanetModel.GetOrientation` (the planet surface frame at each
slot) instead of the layout quaternion, and apply re-asserts position, scale,
and orientation after the game's own `AddBuilding`/`SetObject` calls. The
all-colonies action only targets colonies on the same planet as the open
planner colony (a distance guard against the planner colony's position; the
first attempt used `PlanetModel.ToSurface` and crashed in-game) and
processes the open planner colony last. The pattern format version is bumped
to 2. Copying reports its counts, and every copy/apply appends crash-safe
diagnostic lines to `colony-copy.log`/`colony-apply.log`.

Runtime 0.5.10 responds to the first in-game tests of 0.5.9. `FastDialogueOpening`
is disabled by default: the 22:02:52 report faults inside the game's effect code
called from `MakeCommOpeningInstant` when it walked the communication panel's
window-procedure list, so the cosmetic opening speed-up is compiled out of the
standard build (the code remains, now guarded with a best-effort `__try` walk,
for an experimental `FastDialogueOpening = $true` build). Applying a pattern no
longer destroys and recreates a target object that already has the same noun:
the first such replacement - an ornament at decoration slot 4 - crashed in
`GameNounManager::CreateInstance` (22:07:12), so matching slots are updated in
place and the destroy/create path is used only when the noun must change.

Runtime 0.5.11 re-enables the instant communication opening with per-call fault
guards. `MakePanelInstant` now wraps the window-procedure walk, the
`IGlideEffect` cast, `SetTime`, `SetOffset`, and `Revalidate` in individual
`__try/__except` blocks, so a call that faults in a transient screen state is
skipped rather than crashing the game, and the whole attempt retries for up to
three seconds after a screen becomes active. Successful applications and faults
are recorded in `%APPDATA%\Spore\ERKEK2000_QoL\comm-open.log`.
`FastDialogueOpening` defaults to `$true` again.

Runtime 0.5.12 corrects the effect calls. The March2017 Ghidra type database
shows the game's `IGlideEffect` vtable is seven entries (`0x10 ToWinProc`,
`0x14 GetOffset`, `0x18 SetOffset`) and `IBiStateEffect` places `SetTime` at
`0x18`, but the SDK models `IGlideEffect` as deriving from `IBiStateEffect`, so
MSVC emitted `SetTime` at `0x18` (the game's `SetOffset` - the original null
read) and `SetOffset` at `0x40` (past the vtable - the four logged faults).
`SetTime` now goes through the `IBiStateEffect` interface and `SetOffset`
through `IGlideEffectGameLayout`, a mirror of the game's real IGlideEffect
vtable; both compile to `call [eax+18h]` in their own vtable.

Runtime 0.5.1 persists the latest copied pattern at
`%APPDATA%\Spore\ERKEK2000_QoL\colony-pattern.bin`. Copying atomically replaces
that file, and later launches load it before the planner is opened, so the
apply buttons are immediately available across restarts and different saved
galaxies. The binary format is versioned and checksummed; strict slot, object,
cost, scale, and finite-number checks cause corrupt or incompatible files to
be ignored rather than instantiated.

Runtime 0.5.2 changes the custom UI lifecycle after two 0.5.1 loading-screen
crashes. Planner windows are no longer children of the main UI from startup;
they are created lazily only while `mpCommunityEditor` is active and disposed
when it closes. The crash evidence and reverse-engineering rationale are kept
in `REVERSE_ENGINEERING.md`.

Runtime 0.5.6 adds `CropCircleUplift`. A successful Crop Circle ground or
water hit on an eligible non-homeworld queues one persistent uplift step;
repeat hits for that planet are ignored. Each step uses the configured
`Values.CropCircleUpliftIntervalSeconds` (1200 by default, ten times the
native Monolith mean interval). Creature planets become Tribe after one step,
Tribe planets become Civilization after one further step, and Civilization
planets receive an Empire through `cStarManager::GetEmpireForStar()` before
native Empire-level planet data is generated. Progress lives
outside saves at `%APPDATA%\Spore\ERKEK2000_QoL\crop-circle-uplift.bin` and is
validated by magic, version, exact size, count limit, and checksum. Monolith
logic and all homeworlds are deliberately untouched.

Runtime 0.5.6 also adds `FastDialogueOpening`. The previous package-generated
`CommScreen-3.spui` override is still omitted because it crashed even without
the DLL. Instead, the runtime detours the native communication-event display
and updates the two existing `IGlideEffect` instances on the native panel
windows to zero time and zero offset. The original screen remains the sole UI
owner; this changes only its opening animation.

Runtime 0.5.3 follows a third, equivalent crash in 0.5.2. The earlier fix left
four custom procedures attached to the root UI during stage loading. Colony
pattern, colony-building, Space-hotbar, and communication UI handlers now use
Space-game update guards, attach only after Space stage becomes active, and
detach outside it. Colony-pattern handlers attach only to their own buttons,
not the root window. This is a static lifecycle fix pending the in-game
regression test in `TESTING.md`.

Runtime 0.5.4 follows a fourth, identical crash in 0.5.3, whose minidump
symbolizes to `UTFWin::Window::AddWinProc` calling a stale procedure through the
window's procedure list (`UTFWin::Window::func64`) while the game rebuilds the
UI tree at the end of a planet load. Every Space-only procedure now attaches
exactly once, only when `IsSpaceGame()` is true and `IsLoadingGameMode()` is
false, and is never detached during the session; no `RemoveWinProc` is ever
called outside DLL dispose, and the main window is never cached across frames.
The colony-pattern controller is kept alive for the session and the planner
buttons are created strictly while the planner is open. `App::ConsolePrintF`
replaces `SporeDebugPrint` (a Release no-op) so attach events reach
`spore_log.txt`. A crash-isolation build with all UI features compiled out is
kept in `dist/variants/NoUI-0.5.4/` for the A/B test documented in
`TESTING.md`.

The configuration generates a package before launch; it is not a runtime file.
The build validates option names and types, stages changes in a temporary
directory, verifies the configured compiled resources, and only then replaces
the release package. This configuration covers every package-side
ERKEK-specific delta. Bio Protector immunity and colony pattern controls are
implemented separately by the runtime DLL; other runtime-only requests remain
listed in `ON-HOLD.md`.

The remaining items from the specification's "POSSIBLY HARD TO IMPLEMENT
FEATURES" section are accounted for in
[ON-HOLD.md](ON-HOLD.md).
