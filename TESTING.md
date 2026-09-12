# Verification and play-test checklist

The compact validation checklist is in [`TODO.md`](TODO.md). Use the detailed
procedures in this file when testing a release build.

## Starting-homeworld spice test

Start several disposable new games and confirm each newly generated homeworld
receives a valid spice selected from the game's space-trading spice list. The
native red result should be replaced when another eligible spice exists, and
the color shown in Space view must agree with the assigned spice. Then load an
existing save whose homeworld already has a spice assigned and verify the
runtime leaves both its spice key and displayed color unchanged. The hook is
limited to an unassigned homeworld during native spice assignment; include an
existing save with an empty spice key in the test if available.

## Runtime Bio Protector test

Static compilation verifies the detour signature against the public ModAPI SDK,
but actual event selection must be tested in-game. Use a disposable galaxy,
place a Bio Protector on one player colony, and leave a comparable colony
unprotected. Confirm future eco-disasters can still target the unprotected
colony but never the protected one. A disaster already active when the
Protector is installed is expected to continue.

## Runtime dialogue shortcut test

Open ordinary planetary communication and verify ESC and TAB each activate the
same Goodbye path as clicking the button. Repeat inside trade, mission, and
diplomacy submenus: when Goodbye is hidden or disabled, the shortcut must not
skip confirmation or discard state. Verify TAB no longer changes focus when it
successfully closes communication, and that both keys retain normal behavior
everywhere outside communication.

## Runtime dialogue-opening speed test

Open a normal Space-stage planetary communication. The native screen should
appear immediately in its final position, with its usual text, trade, mission,
and Goodbye controls intact. Repeat after closing and reopening several
communications, including a trade and mission dialogue. Enter a fresh Space
game and confirm the loading screen remains stable; the package must still
omit `CommScreen-3.spui`.

## Galaxy-map spice collection test

Leave known amounts of two spice colors on multiple player colonies in one
system, then reach that star in the galaxy map without entering it. Confirm the
accepted amounts appear in cargo and are removed from their source planets.
Repeat with a partially full matching stack, no free slot for a new spice
color, and a completely full stack; overflow must remain on the planet. Leave
and return to the same star to confirm the once-per-arrival guard rearms.

## Runtime cargo-limit repair test

Use a backed-up copy of the old save that previously stopped at 99. Enter Space
stage and collect across 98 -> 99 -> 100; verify the cargo panel and trade UI
both show the same quantity. Sell one and several units, buy units, approach
999, test overflow, save/reload, and repeat a transaction. Repeat the basic
test in a fresh disposable galaxy. The runtime uses the game's own inventory
setter, but this behavior is not considered proven until these checks pass.

## Colony building shortcut test

Open the planner for an owned Space-stage colony. Confirm the central keyboard
hook is reported as attached in the game console. Verify **1** selects House,
**2** Entertainment, **3** Factory, and **4** Turret exactly as clicking the
corresponding visible palette item, including the normal saved-design and
placement flow. Test a disabled or unavailable item and confirm the shortcut
does not bypass it. Close the planner and verify the same keys retain normal
game behavior. Also verify Ctrl/Alt/Shift plus a number is not consumed.

## Space hotbar keyboard test

Confirm the central keyboard hook is reported as attached. Outside the colony
planner, open every Space tool tab in turn and press the
number printed for each visible slot. Verify the corresponding tool or cargo
item follows exactly the same selection/use path as clicking it, including
disabled, unavailable, depleted, and recharging slots. Test **1** through
**9**, plus **0** if a panel exposes a tenth slot. Confirm hidden tabs do not
react, Ctrl/Alt/Shift plus a number remains untouched, and switching tabs
immediately changes which native hotbar receives the key. Finally open the
colony planner and confirm **1** through **4** still select buildings rather
than Space tools.

## Colony pattern test

Use only a backed-up disposable galaxy. Open an owned Space colony and confirm
the three new buttons are visible, with both apply buttons disabled before the
first copy. Confirm that they are absent before opening the colony editor and
immediately after closing it. Build a distinctive mix of houses, entertainment, factories,
turrets, civic decorations, and empty slots, then copy it. Confirm both apply
buttons enable.

Open the homeworld colony editor and use either apply action. Confirm that no
objects are changed and the warning `Saved patterns cannot be applied to a
homeworld.` appears; homeworld layouts are intentionally excluded because they
are not compatible with regular colony templates.

Hover each apply button. Verify the displayed required Sporebucks matches one
colony or all colonies respectively, is green when the current balance covers
it, is red otherwise, and always includes the warning that construction runs
only until funds run out. Apply to a partially occupied colony while a palette
building remains selected. Confirm there is no crash: same-noun structures are
updated in place, empty target slots can be filled, and an occupied slot whose
noun differs from the saved pattern remains unchanged. Saved empty slots must
not demolish target buildings, turrets, or decorations. Confirm orientations
follow the target colony and the shown cost excludes preserved mismatched slots.

Repeat with too little money. Confirm affordable slots build in order,
unaffordable occupied slots remain unchanged, the balance never becomes
negative, and `Build incomplete: funds ran out.` appears. Test **Apply to all
colonies** with at least two colonies and verify no colony on another planet is
changed. Finally accept/close the planner, save, reload, and confirm the changed
colonies persist without missing or duplicated objects. Also cancel a planner
session once and document whether direct pattern changes follow the game's
normal cancel behavior; do not use this build on the main galaxy until that is
confirmed.

Clear the current palette selection and apply again. The tool should choose an
enabled, visible Sporepedia entry compatible with the saved layout's building
or turret category; it must stop cleanly with a status message when no
compatible entry is available. Confirm newly created buildings and turrets
advance the Colonist badge count once each, while in-place updates and
decorations add no progress.

After copying, confirm
`%APPDATA%\Spore\ERKEK2000_QoL\colony-pattern.bin` exists. Close Spore fully,
restart it, open the planner, and confirm both apply buttons are already
enabled without copying again. Repeat in a separate disposable galaxy and
verify the same pattern can be applied there. Preserve a copy of the pattern
file, damage several bytes in a duplicate test file, and confirm the DLL safely
ignores it and leaves the apply buttons disabled; restore the valid file
afterward.

### Colony pattern city-hall protection (0.5.8)

Copy a colony whose city hall occupies a building layout slot, then apply the
pattern to the same colony and to another. The city hall must remain in place
in every city: applying must not remove, recreate, or move it, and the apply
must finish without a crash. Also confirm that a pattern copied with 0.5.8
saves and reloads correctly, including the protected hall slot (stored as kind
4). Before 0.5.8 the in-memory copy recorded the hall as an ordinary building
because it derives from `cBuilding`, so applying destroyed it and corrupted the
city; see `REVERSE_ENGINEERING.md`. The apply writes one
`ERKEK2000 QoL Runtime: colony pattern slot N kind K ...` line per slot to
`SporebinEP1\spore_log.txt`; the last line before a crash names the failing
slot.

### Colony pattern orientation and hardened apply (0.5.9, updated in 0.5.10)

0.5.9 rejects patterns written by older versions (format version 2); copy the
pattern again after updating. Copy a colony and confirm the status text reports
the counts (`Copied N buildings, M decorations, K turrets.`) and that they
match what the colony actually contains. If the message includes the layout-slot
warning (`some objects are not in layout slots`), the colony holds objects that
cannot be captured; those are the expected cause of a partial copy and must be
reported. Copying must not crash, and moving the mouse over the apply buttons
after a copy must not crash (both were fixed in 0.5.9/0.5.10).

Apply the pattern to a colony that already has buildings and ornaments in the
same slots. Same-noun objects should be updated in place, with `update slot N
noun ...` lines in the apply log; no occupied object should be removed or
replaced. Confirm mismatched occupied objects remain intact. Confirm the pasted
buildings, turrets, and decorations are upright and face the target colony's
direction (not sideways), including when pasting into the same colony. Confirm
that a target colony holding a special
structure (Bio Protector or any object whose slot the pattern does not
recreate) keeps that structure: its slot must be left unchanged.

Confirm clicking a colony in Space opens its communication screen with no crash
and the opening animation plays instantly (0.5.11). Check
`%APPDATA%\Spore\ERKEK2000_QoL\comm-open.log` after opening a few screens: each
line reports how many effects were made instant and how many calls faulted. If
the animation is still at native speed, the log explains whether the effects
were found (a fault count with zero effects) or the walk never reached them.

Use **Apply to all colonies** with at least two colonies, including the open
planner colony, and confirm no crash occurs, every current-planet player
colony is rebuilt (the open one last), and colonies on other planets are not
touched. After copying, move the mouse over each apply button and confirm the
hover text appears without a crash (a 0.5.9 interim build crashed in
`cPlanetModel::ToSurface` on this hover path). If any apply still crashes,
read the last lines of
`%APPDATA%\Spore\ERKEK2000_QoL\colony-apply.log` (and `colony-copy.log` after
a copy) and report them; those files are flushed per line and name the exact
city, slot, noun, and destroy/create step.

## Crop Circle uplift test

Use a disposable, backed-up galaxy. On a non-homeworld Creature planet, use
Crop Circles once and confirm a second use does not create another countdown.
After the configured interval (1200 seconds by default), revisit or reload the
planet and confirm it becomes Tribe; after one further interval it becomes
Civilization; and after a third interval it becomes a native Empire. Confirm
the progress remains after closing and reopening Spore. Crop Circles must have
no effect on a homeworld or already-Empire planet, and using a Monolith must
retain its normal behavior and timing.

### Loading-crash regression

Runtime 0.5.5 removes the generated `CommScreen-3.spui` override from the
default package. The `PackageOnly` control, which contains the same package
but no DLL, reproduced the loading-screen crash after a new Space civilization
was created. This rules out the runtime DLL and makes the custom communication
screen override the first package-level suspect; native planetary dialogue is
now the safe default pending in-game confirmation.

Runtime 0.5.1 failed its first in-game entry test twice: selecting a new planet
and Space civilization reached the loading screen, then crashed at 11:08:04
and 11:09:12 on 2026-09-10. Runtime 0.5.2 delayed the planner windows, but an
equivalent crash recurred at 12:39:51. Runtime 0.5.3 kept all Space-only UI
procedures detached until `Simulator::IsSpaceGame()` was true, yet an
identical crash recurred at 12:52:55 on the first 0.5.3 test.

All four exception reports and minidumps show the same deterministic signature:
inside `UTFWin::Window::AddWinProc` (SDK March2017 symbol), the window-procedure
list walker (`UTFWin::Window::func64`) executes a call through a stale pointer.
The faulting instruction address is freed-heap garbage in the first three
crashes (0x1c4daf44, 0x1c207eac, 0x1ad0c4d4) and `0x00000000` in the 0.5.3
crash (the freed memory had been zeroed). `EDI = 0x05941bb0` is identical in
all four reports. The crash occurs while the game rebuilds its UI tree during
the planet-load transition, only when `ERKEK2000_QoL_Runtime.dll` is loaded.

Runtime 0.5.4 treats the loading window as untouchable: every Space-only
procedure attaches exactly once, only when `IsSpaceGame()` is true **and**
`IsLoadingGameMode()` is false, and is never detached again during the session
(no `RemoveWinProc` churn). The colony-pattern buttons are created only while
the planner is open, with a fresh `GetMainWindow()` each time, and the window
reference is dropped as soon as the planner closes. Testing disproved the UI
lifecycle as the cause: full 0.5.4 crashed identically at 15:24:29, and the
NoUI variant (all four UI features compiled out) crashed identically at
15:28:37. The UI features are exonerated; the same signature now points at the
three data hooks (bio-protector detour, cargo-limit update, galaxy spice
collector) or the DLL shell itself.

Isolation variants are built in `dist/variants/`: `Empty` (no runtime features,
only the no-op DLL shell), `BioOnly`, `CargoOnly`, `SpiceOnly` (each with a
single data hook), `NoUI-0.5.4` (all three data hooks), and `Full-0.5.4`.

Test the ladder in this order on the same disposable save, three planet entries
per variant:

1. `Empty` — stable means a data hook is responsible; a crash means the DLL
   shell, the package, the ModAPI launcher, or vanilla itself is responsible
   (then retest with no ERKEK mods installed at all, and try another planet or
   a fresh galaxy to rule out save-specific vanilla behavior).
2. `BioOnly` — the `CreateMission` detour is attached from process start and
   is active during every load; the prime hook suspect.
3. `CargoOnly` — `SetMaxCargoAmount(999)` fires on the first frame where
   `IsSpaceGame()` is true, which may be inside the load window; second prime
   suspect.
4. `SpiceOnly` — inert outside galaxy-map context; expected stable.

Any crash is a failure; stop after the first one and retain its exception
report and minidump. Selecting another Civilization planet in the shared
galaxy is a valid regression test; creating a separate galaxy is not required.

## Completed static checks

Built using the installed SporeModder FX packer, then reopened the result with
SMFX's DBPF parser. `tools/VerifyPackage.java` checks the actual compiled data:

- All 477 original Kisu resource IDs are retained in this build.
- No duplicate IDs and no unexpected additions/removals.
- All 110 Kisu tiered badge overrides retained.
- Configured restored properties match the staged source and the decoded
  vanilla PatchData values. All other fields in those resources match Kisu.
- 471 inherited resources are byte-identical to Kisu, including the Captain's
  cargo reward, the GA package priority signature, model, PNGs, and localization.
- One editor-only name dictionary is regenerated by SMFX.

See `reports/verification.txt` and `reports/SHA256.txt` for the build output.
The ZIP installer was reopened separately: it contains exactly the compiled
package, x86 runtime DLL, and a valid Galactic Adventures `ModInfo.xml`. The
embedded package and DLL SHA-256 values match the standalone files. The DLL
imports `SporeModAPI.dll` and the SDK's `CreateMission` and
`AssignPlanetSpice` addresses. These are static checks, not gameplay tests.

The package foundation from version 0.3.0 additionally tests configuration at
the compiled-package level.
The default all-enabled configuration reproduces the verified 0.3 gameplay
payload. The included `configs/KisuBaseline.psd1` preset was also built and
verified: it retained all 477 Kisu resource IDs, kept all 110 badge overrides,
and omitted the communication override. The final distributed artifacts are
rebuilt from the default configuration, not that test preset.

`tests/CustomValues.psd1` was built separately with deliberately non-default
values for all seventeen numeric fields. Verification compared each compiled
property against the staged configured source and passed. Its runtime DLL also
compiled with the deliberately non-default cargo limit of 321. The distributed
artifacts were rebuilt afterward with the default limit of 999. This fixture is
for build testing and is not a recommended gameplay preset.

## In-game checks still required

Use a backed-up test galaxy and install only this overhaul (no Kisu or separate
CargoStack999 package). Run Galactic Adventures through the ModAPI Launcher.

1. Check base colony building prices without applicable discounts: house
   25,600; entertainment 12,800; factory 19,200; turret 16,000.
2. Activate the shield and check its 180-second recharge behavior. Distinguish
   the recharge timer from the period during which the shield is active.
3. Inspect badge requirements against vanilla. Already-earned badges are not
   expected to be revoked. Check the extra Captain cargo reward on a suitable
   new Space-stage game and the availability of Kisu's cargo upgrades.
4. Compare spice production on equal colony layouts, happiness, and difficulty.
   The general rate should be vanilla. The default release intentionally keeps
   Kisu's increased homeworld multiplier; a build with
   `RestoreVanillaHomeworldSpiceProduction` enabled should instead use 0.025.
5. Complete the runtime cargo-limit repair test above. A display stuck at 99 is
   a failure even if pickups appear accepted.
6. Check core travel ranges. Verify Grox placement separately in a fresh galaxy;
   existing stars/empires cannot demonstrate regeneration changes.
7. Smoke-test inherited content: white spice identification/sale, upgrade icons,
   cargo pages, trade bundles, sculpting tools, and dialogue text.
8. Complete the Space hotbar keyboard test above in every tool tab.
9. For a build with `RestoreVanillaSpiceStorageCooldown` enabled, use a Spice
   Storage tool and confirm its recharge takes 30 seconds (or the configured
   value). The default release should retain Kisu's 10-second cooldown.
10. Enable the Happiness Booster, Loyalty Booster, Uber Turret, and Embassy
    cooldown switches. Use each tool and confirm its recharge matches its own
    configured value. The default release should retain 10 seconds for each.

Do not spend a long playthrough on a new galaxy until the cargo test succeeds
and the provisional Grox spread-radius choice is confirmed. The deferred
features in `ON-HOLD.md` are not expected to work in this release.
