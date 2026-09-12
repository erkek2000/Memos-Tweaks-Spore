# TODO, Open Decisions, and Test Plan

This is the single work queue for the runtime edition. A feature can compile
and pass package verification while still needing an in-game test. Unless a
result is recorded here or in [`TESTING.md`](TESTING.md), treat gameplay
validation as pending.

## In-game validation still required

Run these checks with the current default build through the Spore ModAPI
Launcher. Use a backed-up disposable galaxy for any direct colony-pattern
mutation. Do not test the pattern tools first on the primary save.

| Status | Feature | Acceptance check |
| --- | --- | --- |
| Pending | New-game homeworld spice | Start several new games and confirm the starting homeworld receives a valid random spice, including non-red results. Confirm the color shown in Space view matches the assigned trading spice. |
| Pending | Existing-save homeworld spice | Load an existing save whose homeworld has spice assigned; confirm neither spice key nor displayed color changes. Include a save with an unassigned homeworld if one exists, since the runtime condition is an empty spice key. |
| Pending | Kisu badges | Verify Kisu's 110 tiered overrides are in the default package, requirements and rewards appear in-game, and the extra Captain cargo reward and Kisu cargo upgrades remain available. Already-earned badges are not expected to be revoked. |
| Pending | Package balance changes | Check House 25,600; Entertainment 12,800; Factory 19,200; Turret 16,000; Shield recharge 180 seconds; general spice production against a comparable vanilla layout. Distinguish recharge from shield active duration. |
| Pending | Galaxy generation | Confirm core travel restrictions in a fresh game. Verify Grox exclusive radius and spread in a newly generated galaxy; existing galaxies cannot validate generation changes. |
| Pending | Bio Protector | Place a Bio Protector on one colony and leave a comparable colony unprotected. Future eco-disasters may affect the unprotected colony but must not target the protected one. A disaster already active is expected to continue. |
| Pending | ESC/TAB communication close | On ordinary communication, ensure each key activates the visible native Goodbye action. Repeat in trade, mission, and diplomacy submenus; hidden or disabled Goodbye must not bypass their normal confirmation. Verify TAB no longer changes focus after closing and both keys behave normally elsewhere. |
| Pending | Fast dialogue opening | Open and reopen ordinary, trade, and mission communication. The native screen should appear in place with its usual controls. Enter a fresh Space game and confirm loading remains stable. The generated `CommScreen-3.spui` override must remain omitted. |
| Pending | Galaxy-map spice collection | Leave known spice quantities on multiple player colonies in one star system and arrive at the star from the galaxy map without entering the system. Verify accepted spice enters cargo and is removed from planets. Repeat with a partial stack, no free cargo slot, a full stack, and a later return to the same star; overflow must remain and collection must re-arm on a new arrival. |
| Pending | Cargo maximum | On a backup of the affected old save, test 98 → 99 → 100, panel/trade display, pickup, buy, sell, overflow near 999, and save/reload. Repeat the basic check in a disposable new galaxy. Any display or transaction still capped at 99 is a failure to investigate. |
| Pending | Planner building shortcuts | In an owned colony planner, confirm 1 House, 2 Entertainment, 3 Factory, and 4 Turret using the normal visible palette flow. Test unavailable items, keys outside the planner, and Ctrl/Alt/Shift combinations. |
| Pending | Space tool hotbar keys | Test every Space tool tab. Unmodified 1–9 and 0 where a tenth slot exists must act only on the visible native panel and respect disabled, depleted, recharging, cargo, and tab-switching states. Modified keys must remain untouched; planner 1–4 must keep building priority. |
| Pending | Colony pattern safety | Follow the complete disposable-save matrix in `TESTING.md`: copy a mix of supported objects and empty slots, apply to one and multiple colonies, verify occupied mismatched slots are preserved without a crash, and check costs/funds/orientation/model, protected structures, random compatible Sporepedia selection, badge progress, and diagnostic logs. Confirm no colony outside the active planet is changed. |
| Pending | Pattern editor transaction | After applying a pattern in the planner, test both accept and cancel. Determine whether direct changes follow the game's undo/cancel behavior. The public SDK does not expose a verified editor transaction/rollback API. |
| Pending | Crop Circle uplift | On a disposable non-homeworld, confirm the first successful ground/water hit queues one 1,200-second step, repeat hits do not stack, and stages progress Creature → Tribe → Civilization → Empire across three steps. Verify progress survives restart and that homeworlds, Empires, and Monolith behavior are unchanged. |
| Pending | Inherited Kisu assets and tuning | Smoke-test white spice and sale, cargo pages and upgrade icons, trade bundles, sculpting tools, and retained dialogue text. Confirm Kisu's 10-second colony-tool cooldowns and increased storage/caps where relevant. |

Use [`TESTING.md`](TESTING.md) for the expanded procedures, including the
colony crash-regression sequence. Record the date, build hash, save type, exact
steps, and result before marking an item complete.

## Unimplemented requested features

### System purchase valuation from spice and buildings

The requested price contribution per spice planet is:

| Spice | Contribution |
| --- | ---: |
| Red | 500,000 |
| Yellow | 750,000 |
| Blue | 1,000,000 |
| Cyan | 2,000,000 |
| Pink | 3,000,000 |
| Purple | 5,000,000 |
| White | 10,000,000 |

Add half the player's construction cost for each building. Purple is 5M in the
current specification; the older note saying 10M is superseded. The game has
fixed capture-offer prices up to 10M and exposes no inspected property that
sums spice value with individual construction value. Changing only a building
factor or adjusting money after capture would desynchronize the displayed
offer, affordability check, and amount paid.

Next work: inspect the live Steam game's offer-generation path before
`cEmpire::CaptureSystem`; define whether all planets/moons or only settled
planets count, how turrets and discounts are priced, and how the system's
normal negotiation is preserved above 10M. See the valuation research in
`REVERSE_ENGINEERING.md`. Do not ship a partial pricing approximation.

### Galaxy-map spice pickup feedback

Add an appropriate collection animation and/or sound when spice is collected
at a galaxy-map star. Identify a safe native effect path and ensure feedback
occurs only for accepted cargo, not overflow left on colonies.

### Draggable hotbar

The current feature activates the game's existing per-tab Space hotbar with
number keys; it does not add the requested drag-and-drop assignment system.
Investigate whether the native panel already supports persistent dragging and
whether a custom assignment format is needed. Preserve the current native
selection, cooldown, availability, and colony-planner key behavior.

### Colony-tool purchase costs

The earlier task note says “revert colony tools being cheaper in colonies and
home,” but does not identify the affected tools, price fields, or whether the
change should cover both colony and homeworld contexts. Inventory the Kisu
price changes and compare them with vanilla before editing package values.

### Wider configuration coverage

The release has build-time switches for all ERKEK-specific package changes
and implemented runtime features. It does not have live reload, an in-game
settings screen, or a switch for every inherited Kisu feature. White spice
and cargo upgrades span multiple assets, lists, and trading resources; a safe
off switch needs dependency rules and a decision about items already owned.
Decide whether broader Kisu toggles are still wanted before expanding the
configuration model.

## Safety and design constraints

- Keep the generated package-side `layouts_atlas~/CommScreen-3.spui` override
  disabled. A package-only build reproduced a loading-screen crash. Native
  communication animation is changed by the runtime code instead.
- Do not add a load-time spice reroll. The current homeworld hook only changes
  a homeworld during native assignment when its spice key is empty and the
  call is not a forced reassignment; validate the new-game boundary in-game.
- Do not destroy the city hall or any unsupported colony noun from the
  pattern path. Protected slots and unsupported target objects are skipped.
- Do not run planner mutations on the user's primary galaxy until editor
  accept/cancel behavior and the full apply matrix have passed on a backup.
- Never infer that an old save will adopt galaxy-generation changes. Existing
  star placement, earned badges, and prior purchases are persisted.

## Resolved requests and issues

- Kisu badge overrides were restored to the default build at the user's
  direction; the prior “consider reverting the Frequent Flyer badge” note is
  superseded by retaining Kisu's full badge set.
- Colony-pattern paste adds one Colonist (`ReqPlanetsColonized`) progress count
  for each newly created building or turret. It does not count existing-object
  updates or decorations.
- Apply-to-all skips the editor-only building refresh for off-screen colonies
  and restricts targets to the active planet's player colonies.
- When there is no selected Sporepedia palette item, pattern apply randomly
  selects an enabled, visible compatible building or turret entry before
  mutation. If none is available, it exits with a status message.
- City-hall protection, safe noun allowlisting, same-noun in-place updates,
  refreshed slot checks, per-slot planet orientation, and crash-safe copy/apply
  diagnostics address earlier colony-pattern crash reports.

## Project maintenance

- Keep this queue synchronized with new feature requests and results in
  `TESTING.md`.
- After any code, config, or package-source change, run the checked full build
  `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-runtime.ps1`
  from the project root and review `reports/verification.txt` and
  `reports/runtime-build.log`.
- Build-test `configs/KisuBaseline.psd1` and `tests/CustomValues.psd1` when
  changing package configuration or its validation logic.
- Before a long new-galaxy playthrough, finish the cargo-limit test and confirm
  the provisional Grox spread-radius choice.
