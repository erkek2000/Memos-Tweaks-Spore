# Features on hold for discussion

None of the features below is implemented in 0.2.0. No extra software was
installed and no runtime DLL was created for this task. "Likely DLL" is a
technical assessment, not proof that every possible package-only solution has
been exhausted.

## Bio Protector prevents all biodisasters on its planet

Inspected `UniverseSimulatorTuning`, `placebioprotector`, the biosphere-collapse
mission, and the available property registry. The exposed tuning includes
global disaster frequency and Bio Stabilizer infection/collapse time bonuses;
no verified property was found that implements the requested per-planet
Bio Protector immunity.

Retain Kisu's disaster settings for now. Increasing a timer to a huge number or
disabling every galaxy disaster would not implement the requested condition.
Next step: identify how disaster targets are selected and how the planet's
Bio Protector flag is checked. A runtime hook may be needed. Decide whether
the rule applies to allied colonies and whether installing a protector should
cancel an already-active disaster or only prevent future ones.

## Building shortcuts: 1 house, 2 entertainment, 3 factory, 4 turret

This request was added to `mod-features.md` during the build and is included
here. First inspect the colony planner's UI controls and existing keyboard
bindings. A resource-only accelerator binding may be possible with SMFX; if
the panel lacks an appropriate handler, a runtime input component is needed.
The building-price properties edited in this release do not implement keys.

Scope these shortcuts to active colony-building mode so they do not conflict
with number-key hotbar use elsewhere. Discuss whether a key selects a building
type for placement or immediately places one; also decide which saved building
design it selects. No speculative bindings have been shipped.

## ESC or TAB closes dialogue

The registry exposes generic `dialogEscButton`/`dialogEnterButton` properties,
but that alone does not demonstrate support in the planetary communication
panel. Inspect the panel's input handler; use existing UI bindings if supported,
otherwise a DLL input hook. Decide which key and whether it should cancel
trades/missions/submenus, where closing may have different consequences.

## Draggable hotbar and number-key item use

Needs persistent slot assignments, drag/drop handling, quantity rendering,
and input dispatch to the selected cargo/tool action. No such system is added
by a cargo-capacity property. Likely a C++/ModAPI UI component plus UI assets.
Decide slot count, planet versus galaxy contexts, per-save versus global slot
storage, and keyboard conflicts before implementation.

## Crop circles slowly uplift a civilization

The vanilla tool uses `spaceToolStrategy cropcircles`; the inspected tool data
does not expose a simple monolith/uplift-speed parameter. Substituting the
monolith tool would change the tool's behavior and appearance rather than add
the requested effect. Likely needs tool logic and saved progress/timers.
Clarify "90% slower": 10% of monolith progress rate (ten times the time), or
90% extra duration. Also define repeat-use stacking and eligible stages.

## Runtime configuration and inherited Kisu feature toggles

Version 0.3.0 implements build-time switches for every ERKEK-specific change.
Live reload, an in-game settings screen, and exhaustive toggles for every
inherited Kisu feature remain deferred. White spice and cargo upgrades span
assets, tools, lists, and trading, so disabling them safely needs dependency
rules and decisions for already-owned items. A configuration file also cannot
make an old galaxy forget initialized or generated state.

## Collect spice while standing on a star in the galaxy map

Requires detecting galaxy-map arrival/proximity, identifying eligible owned
planets, transferring their stored spice into cargo, respecting capacity,
and persisting the decremented production stores. No verified tuning-only
switch was found. Likely a runtime component. Decide owned versus allied
systems, overflow behavior, and whether collection is automatic or key-triggered.

## System purchase price based on spice and buildings

These two related bullets in the specification are one pending feature. The
requested base contribution per spice planet is:

| Spice | Price contribution |
| --- | ---: |
| Red | 500,000 |
| Yellow | 750,000 |
| Blue | 1,000,000 |
| Cyan | 2,000,000 |
| Pink | 3,000,000 |
| Purple | 5,000,000 |
| White | 10,000,000 |

Add half the player construction value of each building. The current file
specifies purple as 5M; the old feature document's 10M is superseded.

`SpaceEconomy` exposes `tradeCaptureCostT0/T1/T2/T3`, building/turret factors,
and a fixed offer-price list, but no inspected property sums planet prices by
spice color. Changing only the building factor would implement an incomplete,
different pricing rule. Likely needs a valuation hook and purchase UI changes,
including support for prices above the existing top 10M offer.

Discuss what "enough trading" means, whether every planet/moon or only settled
ones counts, how buildings/turrets are priced, whether consequence discounts
affect value, and whether any randomness or negotiation remains.

## Known carry-over issue: 999 on an existing save

0.1.0 preserves `spaceEconomyMaxCargoCount 999` and Kisu's cargo resources.
It does not claim to repair your observed 99 display / sell-to-98 behavior.
The earlier explanation that the limit is definitely galaxy-initialized was
not established by inspecting the executable or save structure. Treat it as
a hypothesis pending controlled old-save and fresh-galaxy tests.

Test collection, all displays, partial selling, buying, and save/reload before
deciding whether a DLL is required. If both old and new galaxies fail, creating
a new galaxy is not the solution. Runtime fixes would need to locate the actual
storage limit, display calculation, and transaction handling.

## Tooling if we choose runtime work later

The [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) supports
C++ mods compiled as DLLs. Its source and examples are useful for investigating
these hooks. That development route would require a compatible C++ build
toolchain/SDK; deeper unknown functions may need a debugger or Ghidra. Do not
assume the existing launcher/loader installation includes the development SDK.
Tool installation and DLL development are deferred for the requested discussion.
