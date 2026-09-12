# Implemented Features

This is the feature inventory for the ERKEK2000 QoL Runtime edition. It records
what the current source and default build do, including which behavior comes
from Kisu 1.5, which behavior this project changes, and what still needs a
real game test. The exact checked-in defaults are in [`config.psd1`](config.psd1).

## Build and verification status

- The release is a standalone Kisu-based data package paired with
  `ERKEK2000_QoL_Runtime.dll`; it is installed as
  `dist/ERKEK2000_QoL_Runtime.sporemod` and launched through the Spore ModAPI
  Launcher Kit.
- The latest package verification retained all 477 Kisu resource IDs and all
  110 Kisu badge overrides. It reported 471 inherited resources byte-identical
  to Kisu and verified the changed resources against the project configuration
  and vanilla references. See [`reports/verification.txt`](reports/verification.txt).
- The runtime compiles as a 32-bit Release DLL with the checked Visual Studio
  2022 / v143 toolchain. Static compilation and package checks do not prove
  in-game behavior; outstanding play tests are tracked in [`TODO.md`](TODO.md)
  and [`TESTING.md`](TESTING.md).
- Package configuration is applied at build time. Editing `config.psd1` does
  not change an already built package, DLL, or save.

## Kisu features retained

The default package keeps the Kisu 1.5 baseline except for the explicit changes
listed below. Retained behavior includes:

- All 110 Kisu badge-tier resources across 22 badge families. This includes
  Kisu's changed requirements and rewards and the Captain badge's extra seven
  cargo slots. Already earned badges are not taken away.
- Kisu's 999 cargo-stack tuning and five cargo upgrades.
- Ten trade routes, a 999 spice purchase cap, colony spice storage of 15,
  white spice and its assets, and the separate homeworld spice-production
  multiplier of 0.25.
- Kisu's smaller 300-system Grox empire, increased terrestrial moon/ring
  chances, 10-second colony-tool cooldowns, trade bundles, consequence powers,
  archetype changes, sculpting tools, disaster tuning, and other inherited
  content.
- Kisu's camera, ship, and transition changes, plus the game's existing city
  protection and conquest behaviors.

The specific numerical values and all toggles are documented in the
configuration table below. The untouched decoded Kisu source is kept under
`reference/kisu/`.

## Original feature-request coverage

| Requested behavior | Current state |
| --- | --- |
| Prevent future Bio Protector eco-disasters | Implemented in the runtime DLL; ongoing disasters are not cancelled. |
| Keep vanilla badge requirements | Superseded by the user's later direction: retain Kisu's complete 110-resource badge set. |
| Restore Grox/core distances, general spice rate, colony building costs, and shield recharge | Implemented as package changes with build-time values. |
| Randomize starting-planet spice | Implemented for an unassigned homeworld during native spice assignment; new-game and old-save edge cases need the checks in `TODO.md`. |
| Copy/apply a saved colony pattern, with automatic compatible palette selection | Implemented with copy, apply-to-one, apply-to-all, persistence, cost checks, and protected slots. |
| Use 1/2/3/4 for colony building selection | Handler is present; 0.5.13 was reported broken. Build 0.5.14 added diagnostics; current build 0.5.15 is not yet verified in-game. |
| Open planetary dialogue instantly and close with ESC/TAB/Spacebar/End | In-game testing confirmed the 0.5.21 Spacebar path closes the colony's "Speak with the colony" dialogue, restores movement, zoom, and travel immediately, allows reopening, and leaves the Speak button steady. ESC, TAB, End, and submenu behavior remain unverified. |
| Use number keys to select Space tools | Handler is present; current build has not yet been confirmed in-game. |
| Drag and assign a custom hotbar | Not implemented; see `TODO.md`. |
| Apply a 90%-slower civilization uplift with Crop Circles | Implemented as one non-stacking 20-minute step per successful hit sequence, persisted between launches. |
| Configure every change before starting a save | Package and runtime settings are build-time configurable; live settings and all inherited-Kisu feature toggles are not implemented. See `TODO.md`. |
| Collect stored spice from galaxy-map star arrivals | Implemented with inventory-capacity handling and once-per-arrival behavior. |
| Price system purchases by spice planets and buildings | Not implemented; full requested values and research constraints are in `TODO.md`. |
| Retain Kisu city protection, faster transitions, cargo, trade, and inherited content | Retained with the Kisu package baseline, except for explicit package changes above. |

## Package changes from Kisu

| Area | Default release behavior | Default value or source value |
| --- | --- | --- |
| Badge tiers | Retain Kisu's 110 badge overrides | `RestoreVanillaBadgeRequirements = false` |
| Grox-exclusive distance | Restore vanilla distance | 100 pc |
| Grox star spread | Restore vanilla spread | 105 pc |
| Grox empire size | Retain Kisu's smaller empire | 300 systems (vanilla is 2,400) |
| Terrestrial moons and rings | Retain Kisu's increased chances | Moon 10%; rings 25% |
| Core travel band | Restore vanilla restriction | 70–100 pc |
| General spice production | Remove Kisu's 50% boost | 0.0005 (Kisu is 0.00075) |
| Homeworld spice production | Retain Kisu's multiplier | 0.25 (vanilla is 0.025) |
| House base cost | Restore vanilla cost | 25,600 |
| Entertainment base cost | Restore vanilla cost | 12,800 |
| Factory base cost | Restore vanilla cost | 19,200 |
| Turret base cost | Restore vanilla cost | 16,000 |
| Shield recharge | Restore vanilla recharge | 180 seconds |
| Planetary communication screen | Use the native screen; omit generated `CommScreen-3.spui` | Package override disabled |

The Grox spread restoration is a companion choice to restoring the exclusive
radius. The default empire size stays at Kisu's value. Changing galaxy
generation values does not regenerate an existing galaxy.

## Runtime features

### Homeworld spice on a new game

The runtime intercepts native spice assignment. When a homeworld has no spice
assigned yet, it chooses a random valid spice from the game's space-trading
spice list, excluding the native assignment when another choice exists, and
updates the planet's displayed spice color. A populated homeworld with an
existing spice assignment passes through unchanged. The fresh-game and
existing-save behavior still needs the in-game check listed in `TODO.md`.

### Bio Protector and communication controls

- A Bio Protector on a planet prevents creation of future `biospherecollapse`
  eco-disaster missions for that planet. A disaster already in progress is not
  cancelled.
- ESC, TAB, Spacebar, and End have handlers in the game's central keyboard-input hook, intended
  to activate the communication screen's native Goodbye button only when it is
  visible and enabled. Both were reported to do nothing in 0.5.13. Build 0.5.14
  added diagnostics and relaxed an over-strict UI-container enabled check.
  Build 0.5.15 added unmodified Spacebar, but the dispatched button-click message
  did not close the dialogue; runtime logs showed that the Goodbye button's
  command ID was zero. The 0.5.17 crash dump shows an invalid AddRef while the
  current event was copied from manager offset `0x1C`; the game's event slot is
  at `0x20`. Build 0.5.18 reads the native slot and queues Spore's exit action
  until the key callback returns, then checks the active event and Goodbye
  button again. The user confirmed that 0.5.18 hid the dialogue but left Space
  controls locked and prevented reopening dialogue. Build 0.5.19 also sends
  the key through Spore's native input state machine before the deferred exit.
  The planet's "Speak with the colony" test still left the event active;
  0.5.20 restored controls after one second but made the dialogue button flash.
  Build 0.5.21 consumes the close key and checks for recovery on the first
  update where the CommScreen root is hidden, with a 250 ms retry window.
  In-game testing confirmed Space closes the colony Speak dialogue, restores
  movement, zoom, and travel immediately, allows reopening, and leaves the
  Speak button steady. ESC, TAB, End, and submenu behavior remain unverified.
  Shortcuts run only when Goodbye is visible and enabled; otherwise native key
  behavior passes through.
- The runtime speeds the native planetary communication screen's opening by
  setting its two main vertical entrance glides to zero duration and offset.
  It does not replace the screen resource or change its content and controls.

The custom `CommScreen-3.spui` package override remains disabled because it
reproduced a loading-screen crash in a package-only control. See
`REVERSE_ENGINEERING.md` for crash history and `TODO.md` for required play tests.

### Galaxy-map spice collection

On arrival at a star in galaxy-map context, the runtime collects stored spice
from player-controlled colonies in that system. It runs once per arrival,
respects each spice stack's limit and available cargo slots, and removes only
the quantity the inventory accepted. Overflow stays stored on the source
planet. The feature does not collect when merely selecting a star.

### Cargo limit and keyboard controls

- The runtime reapplies the configured cargo maximum to the live Space-stage
  inventory with the game's `SetMaxCargoAmount()` API. Default maximum: 999.
- In the owned colony planner, the handler attempts to map unmodified **1–4**
  to House, Entertainment, Factory, and Turret palette entries. These did
  nothing in 0.5.13; build 0.5.14 added diagnostics, and the current 0.5.15
  build is not yet verified in-game.
- Outside the colony planner, the handler attempts to route unmodified **1–9**
  and **0** to the visible native Space tool panel. This has not yet been
  confirmed in-game.

The handler no longer depends on the main/root UI window receiving a focused
key message, but that change has not made the reported dialogue/planner
shortcuts work. See the known-failure entries in `TODO.md`.

The specification's draggable custom hotbar is not implemented; these
shortcuts use existing native per-tab tool slots. See `TODO.md`.

### Colony-pattern tools

The active colony planner adds **Copy colony pattern**, **Apply saved pattern**,
and **Apply to all colonies**.

- Copy captures supported building, turret, and civic-decoration slots,
  including empty slots, model and scale, and orientation relative to the
  planet surface at each slot. Unsupported objects such as the city hall are
  marked protected.
- The most recent pattern is stored in
  `%APPDATA%\Spore\ERKEK2000_QoL\colony-pattern.bin`. The format is
  versioned and checksummed; incompatible or malformed files are rejected.
- If no suitable palette item is selected, apply randomly selects an enabled,
  visible Sporepedia palette entry matching the saved layout's building or
  turret category. If no compatible item is available, it shows a status
  message and makes no changes.
- Apply charges for matching objects updated in place and new objects created
  in empty slots, in slot order; it never makes the player's balance negative.
  Protected slots, unsupported target objects, and occupied slots whose noun
  differs from the saved pattern are left in place. Saved empty slots do not
  demolish existing objects. The editor tool never removes live city objects;
  this prevents crashes when the editor still holds a selection or placement
  reference. Copy and apply diagnostics are written beside the pattern file.
- Apply-to-all is limited to player colonies near the active planner colony on
  the same planet; the edited city is processed last. The editor-only building
  refresh runs only for the actively edited city.
- Homeworld application is blocked because its city-layout topology differs
  from ordinary colony layouts.
- Each newly created building or turret adds one count to the Colonist badge's
  `ReqPlanetsColonized` progress. Updating an existing object and placing
  decorations do not add progress.

The editor's public API does not expose a safe transaction/rollback call, so
patterns should be tested in a backed-up disposable galaxy before normal use.
The in-game acceptance, cancellation, persistence, and city-scope checks remain
listed in `TODO.md` and `TESTING.md`.

### Crop Circle uplift

The first successful Crop Circle ground or water hit on an eligible
non-homeworld Creature, Tribe, or Civilization planet queues one persistent
uplift step. Each step takes 1,200 seconds by default, ten times the native
Monolith mean interval. Creature becomes Tribe, Tribe becomes Civilization,
and Civilization becomes a native Empire after a third step. Repeated hits do
not stack or shorten a timer. Monoliths and homeworlds are unaffected.
Progress is stored in
`%APPDATA%\Spore\ERKEK2000_QoL\crop-circle-uplift.bin`.

## Build-time configuration

All switches are in `config.psd1`; every value takes effect only after a full
rebuild. `true` under “Default” means the vanilla restoration switch is
enabled. Values shown are the selected release value when the switch is on;
when it is off, Kisu's value is kept.

| Switch | Default | Effect when enabled |
| --- | --- | --- |
| `RestoreVanillaBadgeRequirements` | false | Omit Kisu's 110 tiered badge overrides |
| `RestoreGroxExclusiveRadius` | true | Use configured 100 pc instead of Kisu's 30 pc |
| `RestoreGroxSpreadRadius` | true | Use configured 105 pc instead of Kisu's 35 pc |
| `RestoreGroxEmpireSize` | false | Use configured 2,400 instead of Kisu's 300 |
| `RestoreTerrestrialMoonChance` | false | Use configured 1% instead of Kisu's 10% |
| `RestoreTerrestrialRingChance` | false | Use configured 10% instead of Kisu's 25% |
| `RestoreCoreTravelRestriction` | true | Use configured 70–100 pc instead of Kisu's 25–35 pc |
| `RestoreGeneralSpiceProduction` | true | Use configured 0.0005 instead of Kisu's 0.00075 |
| `RestoreVanillaHomeworldSpiceProduction` | false | Use configured 0.025 instead of Kisu's 0.25 |
| `RestoreVanillaMaximumTradeRoutes` | false | Use configured 5 instead of Kisu's 10 |
| `RestoreVanillaMaximumSpiceBought` | false | Use configured 200 instead of Kisu's 999 |
| `RestoreVanillaColonySpiceStorage` | false | Use configured 5 instead of Kisu's 15 |
| `RestoreVanillaSpiceStorageCooldown` | false | Use configured 30 seconds instead of Kisu's 10 |
| `RestoreVanillaHappinessBoosterCooldown` | false | Use configured 30 seconds instead of Kisu's 10 |
| `RestoreVanillaLoyaltyBoosterCooldown` | false | Use configured 30 seconds instead of Kisu's 10 |
| `RestoreVanillaUberTurretCooldown` | false | Use configured 30 seconds instead of Kisu's 10 |
| `RestoreVanillaEmbassyCooldown` | false | Use configured 30 seconds instead of Kisu's 10 |
| `RestoreColonyBuildingCosts` | true | Use configured House/Entertainment/Factory/Turret costs |
| `RestoreShieldCooldown` | true | Use configured 180 seconds instead of Kisu's 120 |
| `InstantPlanetaryDialogue` | false | Generate the known-unsafe package screen override; keep disabled |

Runtime DLL switches compiled by `build-runtime.ps1`:

| Switch | Default | Behavior |
| --- | --- | --- |
| `PreventBioDisastersWithBioProtector` | true | Suppress future eco-disaster mission creation on protected planets |
| `CloseDialogueWithEscape` | true | Map ESC to the native Goodbye action |
| `CloseDialogueWithTab` | true | Map TAB to the native Goodbye action |
| `CloseDialogueWithSpacebar` | true | Map unmodified Spacebar to Goodbye when available |
| `CloseDialogueWithEnd` | true | Map End to the native Goodbye action |
| `FastDialogueOpening` | true | Shorten the native communication opening animation |
| `CollectSpiceAtGalaxyStars` | true | Collect stored player-colony spice on star arrival |
| `EnforceCargoStackLimit` | true | Apply `Values.CargoStackLimit` to live inventory |
| `BuildingKeyboardShortcuts` | true | Enable planner 1/2/3/4 building selection |
| `SpaceHotbarKeyboardShortcuts` | true | Enable native Space hotbar number selection |
| `ColonyPatternButtons` | true | Enable pattern copy/apply controls |
| `CropCircleUplift` | true | Enable persistent slow Crop Circle uplift |

Numeric configuration lives under `Values`: Grox exclusive/spread radii,
Grox empire size, terrestrial moon/ring chances, core minimum/maximum,
general and homeworld spice multipliers, four colony-building costs, shield
cooldown, cargo maximum, trade-route limit, spice purchase limit, colony spice
storage, five colony-tool cooldowns, and Crop Circle interval. See the exact
names and defaults in `config.psd1` before editing.

## Related project documents

- [`README.md`](README.md): project entry point, install caution, and full-build
  commands.
- [`TODO.md`](TODO.md): all incomplete requests, explicit risks, and required
  in-game tests.
- [`TESTING.md`](TESTING.md): detailed validation procedures and completed
  static checks.
- [`REVERSE_ENGINEERING.md`](REVERSE_ENGINEERING.md): SDK findings, crash
  analyses, planner and runtime internals, and system-valuation research.
- [`PROVENANCE.md`](PROVENANCE.md): input files, hashes, and build lineage.
- [`runtime/README.md`](runtime/README.md): runtime DLL build component.
