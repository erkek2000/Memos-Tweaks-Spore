# Spore reverse-engineering notes

Read this before repeating SDK, package, Ghidra, or colony-planner research.
These notes separate observed facts from working assumptions because several
planner internals are not exposed by the public ModAPI SDK.

## Tested executable and tool context

- Installed game executable:
  `C:\Program Files (x86)\Steam\steamapps\common\spore 24720\SporebinEP1\SporeApp.exe`
- The exception reporter identifies it as Spore 3.1.0.29. ModAPI reports the
  `March2017` platform and Spore ModAPI 2.5.568.
- Local SDK checkout:
  `C:\Users\nambu\Modding Projects\Spore\Tools\Spore-ModAPI-SDK`
- Ghidra import material is under that checkout's `SDKtoGhidra` directory.
  It was added to the existing sparse checkout. `SporeGhidra_march2017.xml`
  is the relevant symbol/type source for this Steam executable; do not assume
  Disk addresses apply to it.
- `SDKtoGhidra/additional_disk.txt` and `SporeGhidra_disk.xml` contain
  `Simulator_cCommunityEditor_ctor` at Disk address `0xD0C530`. No equivalent
  named constructor was found in the March2017 import. This address must not be
  called or patched in the Steam build.

## Community editor findings

Verified from the current SDK headers:

- `Simulator::cSimulatorSpaceGame` stores the active community editor as the
  still-opaque `int mpCommunityEditor` at object offset `0x20`. A nonzero value
  is useful as an active-planner guard, but it is not a safe typed API.
- The SDK has no public `cCommunityEditor` class. Its transaction, undo,
  accept, cancel, pricing, and placement methods therefore cannot be called
  safely by name.
- `cCity` exposes `mBuildingsLayout`, `mDecorationsLayout`, and
  `mTurretsLayout`. Each `cCommunityLayout` has `mSlots`; each `cLayoutSlot`
  exposes `SetObject()` and `RemoveObject()` plus its target position.
- `cCity` exposes `AddBuilding()`, `RemoveBuilding()`, and
  `ProcessBuildingUpdate()`. It also exposes the building, turret, and civic
  object containers. Turrets and ornaments do not have equivalent public city
  add/remove helpers.
- Simulator objects can be created and destroyed through
  `GameNounManager.CreateInstance()` and `DestroyInstance()`, followed by
  `UpdateModels()`.
- `cBuilding::GetCost()` supplies the live building cost. The package properties
  are `SleepCostSpace`, `EntertainmentCostSpace`, `IndustryCostSpace`, and
  `DefenseCostSpace` in `animations~/CityGameBuildingTuning`. The turret has no
  public `GetCost()` equivalent, so this runtime passes the configured
  `Values.TurretCost` into the DLL. Civic objects use the verified cost 25.
- Player funds are available as `Simulator::GetPlayerEmpire()->mEmpireMoney`.
- `cSpatialObject::mbIsBeingEdited` is available and can identify a city or
  structure involved in the planner. The current implementation first searches
  player cities for this flag, then falls back to the player UFO's nearest
  player city.
- `Simulator::GetData<cCity>()` supplies active city nouns. In Space planet
  view these are treated as the colonies loaded for the current planet. This
  current-planet scoping is a runtime assumption and must be verified in-game.

The saved layout records slot occupancy, noun/definition IDs, model key,
scale, cost, and orientation relative to the source layout quaternion. Relative
orientation is required because copying a world-space quaternion directly to a
colony elsewhere on the planet rotates structures incorrectly.

## Built map (verified 2026-09-10)

`dist/ERKEK2000_QoL_Runtime.dll` and `.pdb` are a full-symbol Release build of
`runtime/ERKEK2000_QoL_Runtime.vcxproj` (`/DEBUG /OPT:REF /OPT:ICF`, no
`/GL`-stripping of the PDB). The installed copy under
`C:\ProgramData\SPORE ModAPI Launcher Kit\mLibs` is byte-identical to `dist/`,
so crash addresses can be symbolized against it.

## Colony pattern apply crash (2026-09-10)

Four crashes in one session localize to the colony-pattern apply path. The
faulting thread of the 20:23:34 report has `EIP=0x73653301`, `EBX=0x8b65a119`,
and an `ACCESS_VIOLATION reading 0x8b65a11d` (= `EBX+4`). The DLL base is
`0x73650000`, so the fault RVA is `0x3301`, which disassembles in the current
build to `push dword ptr [ebx+4]` at the entry of
`` `anonymous namespace'::CreatePatternObject `` — reading `pattern.nounID` from
`PatternSlot*`. The walked stack is `CreatePatternObject` (0x3301) ←
`ApplyLayout` (0x35DD) ← `ApplyToCity` (0x3911, the buildings call) ←
`ApplyPattern` (0x3C05) ← `ColonyPatternController::HandleUIMessage` (0x477E).
`EBX`/`[ebp+8]` is a non-aligned value above 2 GB, i.e. not a valid pointer:
the apply loop read a corrupt `sPattern.buildings._Myfirst` (`sPattern` is the
static global at `0x100149CC`).

Root cause: `CaptureLayout` had no noun validation. The city hall derives from
`cBuilding`, so `GetKind()` classified it as an ordinary `Building`, and the
copied pattern recorded it as slot 0 with noun `0x18EA1EB`
(`cBuildingCityHall::NOUN_ID`, cost 0). Applying then destroyed the city hall
(`slot.RemoveObject` + `cCity::RemoveBuilding` + `GameNounManager.DestroyInstance`)
and tried to instantiate a second one. Destroying the hall corrupts the whole
city, and the corruption surfaces as a wild pattern-vector pointer on a later
slot. The persistent loader's `IsSupportedNoun` allowlist already excluded the
city-hall noun, so a restart silently rejected the file; only the in-memory
copy -> apply path (same session) reached the unsafe mutation.

Fix: a new `PatternKind::Protected` (value 4). `CaptureLayout` records any
unsupported noun — explicitly including the city hall — as `Protected` instead
of a copyable building. `ApplyLayout` skips `Protected` slots and, defensively,
never touches a target slot whose current object is the city hall.
`RemoveExisting` refuses the city hall as well. `IsSupportedNoun` accepts
`Protected` only with noun 0, so new files round-trip; older files that recorded
the hall as a building are still rejected by the loader and require a fresh
copy.

## Colony pattern apply crash, second occurrence (2026-09-10 20:59:29)

The 0.5.8 build still crashed once, on the Apply-to-all action. The 20:59:29
report faults in `ERKEK2000_QoL_Runtime.dll` at `EIP=0x73333421` (base
`0x73331000`, RVA `0x2421`) with `EBX=0xad9ecc72` and an `ACCESS_VIOLATION
reading 0xad9ecc76` (= `EBX+4`). Symbolized against the linker map generated
for this build, the stack is:

- `CreatePatternObject` (0x2410, fault at +0x11: reading `pattern.nounID`)
- `ApplyLayout` (0x3620, +0x136)
- `ApplyToCity` (0x39A0, +0xF1)
- `ApplyPattern` (0x3B80, +0x27E)
- `ColonyPatternController::HandleUIMessage` (0x4720, +0x267)
- `SporeApp.exe 0x008488DD` (update-message dispatch)

This is the identical failure signature as the hall crash: a wild `PatternSlot*`
reaches `CreatePatternObject`. The hall itself was already protected, so the
hall fix covered only one instance of the real problem class: the apply path
manually destroys objects the game cannot survive losing through
`RemoveBuilding`/`DestroyInstance`. Single-apply on the open planner colony is
the same operation the game performs in the editor and does not crash; the
all-colonies path adds non-edited colonies and possibly city records from other
planets (the current-planet scoping of `GetData<cCity>` is unverified).

0.5.9 closes the class symmetrically and instrumented it:

- `IsSafelyReplaceable` mirrors the capture allowlist: apply never destroys a
target object unless its noun is house/entertainment/industry/turret/ornament.
The city hall and any other special noun (scenario buildings, Bio Protector
structures, interactive ornaments, etc.) is left in place and its slot skipped.
`RemoveExisting` enforces the same rule independently.
- `GetPlayerColonies` additionally requires each city to sit on the current
planet's surface (`PlanetModel.ToSurface` distance check), so a city record
from an unloaded planet can never be mutated.
- The open planner colony is processed last in the all-colonies action.
- Slot references into `layout.mSlots` are re-fetched and bounds-checked after
every game mutation, in case the game's add/remove helpers resize the layout
mid-loop.
- Every copy/apply now appends crash-safe lines (opened, written, flushed, and
closed per line) to `%APPDATA%\Spore\ERKEK2000_QoL\colony-copy.log` and
`colony-apply.log`. If a crash recurs, the last lines identify the exact city,
slot, noun, and destroy/create step.

## Pasted orientation (2026-09-10)

The user's `colony-pattern.bin` (written by 0.5.8) records the captured
orientations. Decoded: all eight buildings share one nearly-uniform relative
quaternion, about 159° around a near-+Z axis with small per-building axis tilt;
all eight ornaments are near-identity with small horizontal tilt. The
per-slot wobble matches planet curvature at each slot position, so the game
places colony objects in per-slot planet surface frames
(`cPlanetModel::GetOrientation(position, direction)`), not in the layout
quaternion `field_2C`. Restoring against `field_2C` therefore re-bakes the
source colony's origin frame into a target colony elsewhere on the planet:
pasted structures come out rotated by the angle between the two colonies'
surface frames - the reported sideways pastes. 0.5.9 captures and restores
against `PlanetModel.GetOrientation(slotPosition, layout.mDirection)` on both
sides, and re-asserts position/scale/orientation after the game's own
`AddBuilding`/`SetObject` calls in case those overwrite the values. The copy
dump records both the world orientation and the computed frame for every slot,
so the convention can be re-verified from `colony-copy.log` after the next
in-game copy.

## Copy crash and layout-quaternion ground truth (2026-09-10 21:48)

The first 0.5.9 in-game session crashed once, immediately after a successful
copy, when the mouse entered an apply button. The 21:48:12 report faults in
`SporeApp.exe` at `0x00A6F0EC` reading `0x0002bdc2`; the stack is game code
← our `GetPlayerColonies` (return address RVA `0x2471`, the instruction after
`call cPlanetModel::ToSurface`) ← `UpdateHoverText` (`0x4597`) ←
`ColonyPatternController::HandleUIMessage` (`0x4D1F`, the apply-button
mouse-enter branch) ← the game's update-message dispatch `0x008488DD`.
`ToSurface` was the only new planet-model call in the hover path and faulted
inside the game code, so the current-planet surface check was replaced: the
all-colonies action now scopes cities by distance to the open planner colony
instead, using only the already-proven `cCity::GetPosition()` calls and no
planet-model surface math.

The `colony-copy.log` written by that session is the first in-game ground
truth for the orientation convention:

- `field_2C` is the identity quaternion `(0,0,0,1)` on all three layouts, so
  0.5.8 captured and restored world-space orientations verbatim - the direct
  cause of the sideways pastes between colonies.
- `PlanetModel.GetOrientation(slotPosition, layout.mDirection)` differs per
  slot exactly as the surface normal does, and the city hall's world
  orientation equals its slot frame exactly.
- For every non-hall building, `frame⁻¹ * world` is a pure yaw around the
  frame's local up axis (x and y components are zero to four decimals), so
  the new capture isolates each building's yaw and the new apply restores it
  upright on any colony. The yaws do not correlate with the bearing to the
  city hall, so buildings are not radially oriented; their yaw is whatever
  the game assigned at placement time.
- City positions sit ~505 units from the world origin in planet view, so the
  planet radius is about 505 units; the anchor-distance bound uses 2000 with
  a wide margin.
- The copied colony's containers matched its occupied layout slots exactly
  (9/12 buildings, 18/30 decorations, 0/8 turrets), so capture is complete
  for a healthy colony and the earlier 'does not copy all buildings' report
  was not a capture-completeness problem for this colony.

## Dialogue crash and apply replacement crash (2026-09-10 22:02 / 22:07)

Two further reports from the same 0.5.9 session, both symbolized against the
dist linker map (`base 0x73330000`):

- **22:02:52 - communication screen opening.** `ACCESS_VIOLATION reading
  0x00000000` at `SporeApp.exe 0x0085FFC4`, called from
  `MakeCommOpeningInstant+0x54` (RVA `0x8284`, the return address after the
  first `MakePanelInstant` call). `MakePanelInstant` walks the comm panel's
  window procedures (`IWindow::GetNextWinProc`), casts each to
  `IGlideEffect` (`Cast`, TYPE `0xEF2B293B`), and zeroes `SetTime`/`SetOffset`.
  The faulting game routine sits in the FadeEffect/GlideEffect address
  neighborhood (`0x96EFC4` in SDK base terms). The panel's game-owned
  procedure list was therefore not safe for this generic walk in every state.
  Fix (0.5.10): `FastDialogueOpening` defaults to `$false`, so neither the
  detour nor the per-frame and open-screen walks are compiled into the
  standard build; the walk is additionally wrapped in a best-effort
  `__try/__except` for the experimental flag-on build.
  Update (0.5.11): the feature is re-enabled with fine-grained guards. Every
  game call in the walk (`GetNextWinProc`, the `IGlideEffect` cast, `SetTime`,
  `SetOffset`, `Revalidate`) has its own `__try/__except`, the attempt retries
  for three seconds after a screen appears (so a later, stable attempt still
  applies the speed-up), and results are written to `comm-open.log`. The exact
  faulting call could not be resolved from the report (the faulting offset
  `0x56EFC4` sits between the declared GlideEffect/FadeEffect entries), which
  is why every call is guarded individually instead of removing one.
  In-game result (0.5.11): no crash, but `comm-open.log` reported
  `no effect applied, 4 fault(s)` on every screen, so the guards skipped the
  faults but the speed-up never applied.
- **Root cause of the dialogue fault (found via the Ghidra type database).**
  `SDKtoGhidra/SporeGhidra_march2017.xml` gives the game's real vtables:
  `IGlideEffect__vftable` is 7 entries (`AddRef, Release, dtor, Cast`,
  `0x10 ToWinProc`, `0x14 GetOffset`, `0x18 SetOffset`) and
  `IBiStateEffect__vftable` places `0x18 SetTime`. `GlideEffect` owns four
  separate vtable pointers at object offsets `0x00`, `0x04`, `0x0C`
  (IBiStateEffect) and `0x60` (IGlideEffect). The SDK header models
  `IGlideEffect` as deriving from `IBiStateEffect`, so MSVC places
  `SetTime`/`SetOffset` on an `IGlideEffect*` at `0x18`/`0x40` - but on the
  game's IGlideEffect subobject `0x18` is **SetOffset**, and `0x40` is past
  the 7-entry vtable. The original crash was therefore
  `SetTime(0.0f)` landing in `SetOffset`, whose `Point*` argument read the
  float's zero bits as a null pointer (the `ACCESS_VIOLATION reading
  0x00000000`), and `SetOffset` at `0x40` jumped through a garbage pointer -
  the four faults. Fix (0.5.12): call `SetTime` through the `IBiStateEffect`
  interface, whose SDK layout matches the game exactly, and `SetOffset`
  through a mirror class that reproduces the game's IGlideEffect vtable
  (`IGlideEffectGameLayout`). The compiler now emits `call [eax+18h]` for both
  calls, each in its own interface's vtable.
- **22:07:12 - Apply saved pattern.** `ACCESS_VIOLATION writing 0x00000024` at
  `SporeApp.exe 0x00A10C8E`, called from `ApplyLayout` (RVA `0x39B9`, the
  instruction after `call cGameNounManager::CreateInstance`) ← `ApplyToCity`
  (`0x41B1`) ← `ApplyPattern` (`0x5338`) ← `HandleUIMessage` (`0x5D7F`). The
  apply log's last line before the fault is `destroy noun 018C88E4 object
  256C0DE0` at decoration slot 4, so the game's noun factory faulted when
  creating a replacement immediately after destroying the previous object of
  the same noun. Twelve earlier creates on empty slots in that run succeeded.
  Fix (0.5.10): a target slot that already holds the same noun is updated in
  place (definition, model key, scale, position, orientation, model-changed
  flag); destroy/create is used only when the noun must change. The apply log
  now prints `update slot N noun ...` for those slots.

The apply log from that run also shows the 0.5.9 orientation path working:
`create slot` lines carry the per-slot surface frame, and the copy log's frame
values match the compiled `PlanetModel.GetOrientation` path.

## Supported colony object nouns

Persistent files deliberately accept only the SDK noun IDs for
`cBuildingHouse`, `cBuildingEntertainment`, `cBuildingIndustry`, `cTurret`,
and `cOrnament`. Do not broaden this allowlist merely because another noun can
be cast to `cBuilding`; creating arbitrary or city-hall nouns from a damaged
external file can corrupt a colony.

The SDK's `cTurret` inherits multiple interfaces that each expose AddRef and
Release, but it does not add the disambiguating `using Object::AddRef/Release`
declarations used by other SDK noun classes. Instantiating its EASTL intrusive
container methods therefore fails to compile. `ColonyPattern.cpp` specializes
EASTL's turret add/ref release helpers to use the `cGameData` implementation.

## Planner UI resources and runtime UI

Package inspection found these native planner SPUI resources:

- `layouts_atlas~/plannerPalette`
- `layouts_atlas~/CityPlannerCityStats`
- `layouts_atlas~/PlannerSwatch`

The project tool `tools/DumpSpui.java` can dump their window topology after
extraction. `plannerPalette` has an 800x600 root and contains the native lower
planner palette. No stable public SDK control was found for adding arbitrary
planner actions to its internal controller.

The public UI route uses `UTFWin::IButton::Create()`, `IWindow::AddWindow()`,
button win-procedures, and update/refresh messages. A critical lifecycle rule
was learned from four reproducible crashes:

- Never attach, detach, or otherwise mutate UI procedures on the main window
  while the game is loading or transitioning its UI tree.
- 0.5.1 attached the planner buttons and keyboard procedures at post-init and
  crashed twice on the loading screen at 2026-09-10 11:08:04 and 11:09:12.
- 0.5.2 delayed the planner windows but kept four custom procedures on the
  root UI; an equivalent crash recurred at 12:39:51.
- 0.5.3 attached only after `Simulator::IsSpaceGame()` and still crashed at
  12:52:55, because the mode flips to Space at the end of the load, exactly
  while the game rebuilds the UI tree.
- All four reports are in `SporebinEP1/Exception Report DESKTOP-KATG10S
  09-10-26 *.exception.txt` with minidumps. The stack is deterministic and
  symbolizes to `UTFWin::Window::AddWinProc` (March2017 map) calling the
  procedure-list walker `UTFWin::Window::func64`, which calls through a stale
  pointer: the faulting IP is freed-heap garbage in the first three crashes and
  `0x00000000` in the 0.5.3 crash. `EDI = 0x05941bb0` is identical in every
  report, and the crash only occurs with `ERKEK2000_QoL_Runtime.dll` loaded.
  The crash site is therefore symbolized evidence; which exact object was
  freed but still linked (a replaced main window, or a procedure the mod freed
  while the game still listed it) is still inferred.
- The 0.5.4 rule supersedes the 0.5.3 one: a Space-only `IWinProc` attaches
  exactly once, only when `IsSpaceGame()` is true and `IsLoadingGameMode()`
  is false, and is never detached during the session. No `RemoveWinProc` is
  ever called outside DLL dispose, and the main window is never cached across
  frames. The colony-pattern controller is kept alive for the whole session
  and is added only to the lazily created planner buttons; the window
  reference is dropped whenever the planner closes. `App::ConsolePrintF`
  (not `SporeDebugPrint`, which is a no-op in Release) logs each attach to
  `SporebinEP1/spore_log.txt`.
- Success of this rule still needs the repeated in-game regression test in
  `TESTING.md`, including the NoUI crash-isolation variant.

`SporeTooltipWinProc` exposes plain text but no verified rich-text color span.
To meet the colored-cost requirement, the implementation uses a dedicated
hover window whose entire caption is green or red. Do not claim inline tooltip
markup support without an in-game or binary verification.

## Persistent pattern format

The latest pattern is stored outside any galaxy at
`%APPDATA%\Spore\ERKEK2000_QoL\colony-pattern.bin`, making it available to
different saves and later launches. It is not written into Spore's save files.

The file uses magic `ERKP`, format version 1, three slot counts, fixed-width
48-byte slot records, and an FNV-1a checksum over all records. Writes go to a
temporary file, flush, and atomically replace the previous file. Loading checks
the exact file size, checksum, version, a maximum of 64 slots per layout,
supported noun IDs, finite quaternion/scale values, scale bounds, and cost
bounds. Invalid files are ignored.

## Native communication opening speed

The earlier package patch changed the two vertical `Glide` procedures in
`CommScreen-3.spui` to Time=0 and Offset=(0,0), but the altered resource caused
the loading-screen crash even without the DLL. The public SDK exposes the same
live effects safely:

- `cCommManager::ShowCommEvent(cCommEvent*)` is the native point after a
  communication screen is displayed.
- The main communication screen has root control ID `0x01C3BB0C`; its two
  sliding panels are `0x05E4E5F0` and `0x05E4E5F8`.
- Each panel owns an `IGlideEffect`, available from `IWindow::GetNextWinProc`.
  `IGlideEffect::SetTime(0)` and `SetOffset((0,0))` are public virtual API
  calls that reproduce the old visual change without serializing, replacing,
  adding, or removing any UI resource or procedure.

Runtime 0.5.7 detours `ShowCommEvent` only to adjust those existing objects;
the normal update path repeats the adjustment once the screen is active in
case a dialogue creates the panels asynchronously. This code runs only after
Space stage loading is finished.

## Crop Circle uplift implementation

Header-verified SDK points used by Runtime 0.5.6:

- `Simulator::cCropCirclesToolStrategy::OnHit(cSpaceToolData*, Vector3,
  SpaceToolHit, int)` is the native success path for the `cropcircles` tool.
  The detour always invokes it first, then queues progress only when it returns
  true and the hit is ground or water. This preserves the native projectile,
  effects, recharge, and failure handling.
- `Simulator::GetActivePlanetRecord()` identifies the target planet during the
  successful hit. Homeworlds and worlds outside `Creature`/`Tribe` are ignored.
- `cStarManager::GetPlanetRecord(PlanetID)` retrieves a queued planet later;
  `cPlanetRecord::FillPlanetDataForTechLevel()` generates the native Tribe,
  Civilization, or Empire data before the record's tech level changes. The SDK
  requires an owning empire for Empire generation; the implementation first
  calls the public `cStarManager::GetEmpireForStar()` to create or retrieve
  that native owner instead of constructing an incomplete record.

The default 1200-second step is ten times `SpaceLivingUniverse`'s verified
120-second native evolution mean interval. A fixed-width, checksummed external
queue stores `PlanetID` and absolute Windows FILETIME due time, so only one
task per planet exists and real elapsed time continues across game restarts.
The handler makes no changes to the Monolith strategy, its tuning, or its
timer.

## System-purchase valuation investigation

The desired system price is the sum of each settled planet's spice contribution
(red 500K, yellow 750K, blue 1M, cyan 2M, pink 3M, purple 5M, white 10M) plus
half of each player construction cost. Normal negotiation must remain, and
prices must not be capped at 10M.

Verified SDK data sources:

- `Simulator::cStarRecord::GetPlanetRecords()` loads every planet and moon in a
  system. Each `cPlanetRecord` supplies its spice key (`mSpiceGen`) and city
  data. These are sufficient to calculate the requested valuation.
- `Simulator::cEmpire::CaptureSystem(cStarRecord*, uint32_t)` is the public,
  final ownership-transfer call. It has no price parameter, so detouring it
  could only alter money after the deal; it cannot make the negotiation UI show
  the correct offer or reject an unaffordable custom price.
- `SpaceEconomy` offers `tradeCaptureCostT0` through `T3`, building/turret
  *count* factors, and a fixed `tradeCaptureOfferPrices` list whose largest
  default entry is 10M. Those properties cannot express spice color or each
  building's construction cost, so changing them would be an incomplete,
  capped approximation and must not be shipped as this feature.

The Steam EP1 executable is not statically usable for this task: its on-disk
`.text` section at known March-2017 SDK addresses (including `CaptureSystem`
at `0x00C8D190`, `cStarRecord::GetPlanetRecords` at `0x00BBA900`, and
`UTFWin::Window::AddWinProc` at `0x00961F10`) disassembles as encrypted/packed
data rather than x86 instructions. ModAPI resolves these addresses after the
game is running, but file-only Capstone or Ghidra analysis cannot recover the
offer-calculation function. Continue with a debugger or a live-memory dump of
the running `SporeApp.exe`; find the caller that constructs the system-purchase
offer before `CaptureSystem`, then expose or detour that routine with its price
parameter. Do not use a post-capture money adjustment: it would desynchronize
the displayed offer, affordability check, and paid amount.

## Remaining risks and required tests

Static compilation cannot prove the opaque editor accepts direct city mutation.
Use only a backed-up disposable galaxy until all of these pass:

- Enter a new and existing save without a loading-screen crash.
- Open and close the colony planner repeatedly and travel between planets.
- Copy, apply, accept, save, and reload without missing or duplicate nouns.
- Cancel the planner after applying and determine whether the direct changes
  correctly roll back. The public SDK does not expose the editor transaction.
- Verify `GetData<cCity>()` changes only current-planet colonies.
- Verify full-cost charging, insufficient-funds partial application, empty-slot
  removal, and tooltip totals.
- Restart Spore and load another galaxy to verify the external pattern file.

## Investigation discipline

- Prefer current SDK headers and March2017 Ghidra imports over remembered
  addresses or Disk symbols.
- Record whether each conclusion is header-verified, package-verified,
  Ghidra-symbol-only, inferred from a crash, or proven in-game.
- Never test direct noun/layout mutation first on the user's main galaxy.
- Keep `TESTING.md` synchronized with new runtime behavior and failures.
