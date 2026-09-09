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

The configuration generates a package before launch; it is not a runtime file.
The build validates option names and types, stages changes in a temporary
directory, verifies the configured compiled resources, and only then replaces
the release package. This configuration covers every package-side
ERKEK-specific delta. The Bio Protector immunity is implemented separately by
the runtime DLL; other runtime-only requests remain listed in `ON-HOLD.md`.

The remaining items from the specification's "POSSIBLY HARD TO IMPLEMENT
FEATURES" section are accounted for in
[ON-HOLD.md](ON-HOLD.md).
