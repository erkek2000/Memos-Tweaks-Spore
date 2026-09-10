# Features on hold for discussion

The features below remain unimplemented in Runtime 0.5.6. Bio Protector
immunity, dialogue closing, galaxy-map spice collection, live cargo-limit
enforcement, colony building shortcuts, native Space hotbar keyboard use, and
saved colony pattern controls, and slow non-stacking Crop Circle uplift were moved out of this list and implemented in
the DLL.
"Likely DLL" is a technical assessment, not proof that every possible
package-only solution has been exhausted.

## Runtime configuration and inherited Kisu feature toggles

The inherited 0.3.0 package system implements build-time switches for every
package-side ERKEK-specific change.
Live reload, an in-game settings screen, and exhaustive toggles for every
inherited Kisu feature remain deferred. Homeworld spice production and all five
colony-tool cooldown reductions now have checked package-side toggles. White
spice and cargo upgrades span
assets, tools, lists, and trading, so disabling them safely needs dependency
rules and decisions for already-owned items. A configuration file also cannot
make an old galaxy forget initialized or generated state.

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

## Runtime investigation constraints

The [Spore ModAPI SDK](https://github.com/Spore-Community/Spore-ModAPI) supports
C++ mods compiled as DLLs. A sparse SDK checkout and the existing Visual Studio
C++ toolchain now build this edition. Deeper unknown functions may still need a
debugger or Ghidra, which has not been downloaded due to the disk constraint.
Unknown hooks are documented in `TODO.md` instead of patched speculatively.
