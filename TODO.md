# Runtime TODO and decisions deferred during autonomous work

Updated 2026-09-09.

## Needs in-game validation

- Confirm the root UI window receives `KeyDown` events while the planetary
  communication screen owns focus.
- Confirm the synthetic Goodbye-button click closes the top-level
  communication screen exactly like a mouse click. It deliberately does
  nothing when Goodbye is hidden or disabled, so trading, mission, and other
  submenus keep their normal cancellation/confirmation behavior.
- Decide after testing whether both ESC and TAB should remain enabled. Both are
  implemented because the specification says "ESC or TAB" and neither choice
  was designated as primary.
- Bio Protector immunity still needs the play test documented in `TESTING.md`.
- Confirm `GetActiveStarRecord()` plus the player UFO's `mbAtDestination` is an
  arrival edge rather than a mere galaxy-map selection. Verify spice removal is
  persisted after saving/reloading and that cargo UI refreshes immediately.
- On a copy of the affected old save, confirm live `SetMaxCargoAmount(999)`
  updates every cargo display and all pickup, buy, sell, overflow, and
  save/reload paths. If any subsystem still clamps at 99, record which one
  before adding a narrower hook; do not patch the save structure speculatively.
- In an owned colony planner, confirm **1** selects House, **2** Entertainment,
  **3** Factory, and **4** Turret through the same normal placement flow as a
  mouse click. Verify the keys do nothing outside the planner, do not select a
  hidden/disabled item, and do not conflict with modified number keys.
- In every Space tool tab, confirm unmodified number keys activate only the
  corresponding slot in the visible native hotbar. Include disabled, depleted,
  recharging, cargo, tab-switching, modifier, and colony-planner checks.

## Deferred design choices

- Auto build: define how a saved city pattern is selected, what happens when a
  slot cannot accept the saved building, and whether purchases may leave the
  colony with negative funds.
- Crop circles: currently interpret "90% slower" as 10% of monolith progress,
  but do not ship that assumption until a safe persisted progress mechanism is
  identified.
- System valuation: count all settled planets in the system, use undiscounted
  player construction costs, retain normal trade negotiation, and replace only
  its valuation baseline. These are provisional defaults pending a verified
  hook and UI support above 10,000,000.
