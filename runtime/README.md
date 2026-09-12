# Runtime component

`ERKEK2000_QoL_Runtime.dll` intercepts creation of the
`biospherecollapse` event. If the proposed source planet's saved
`mPlanetObjects` list contains the `SG_colonytool_bioprotector` model, mission
creation returns null and the future eco-disaster is suppressed.

The DLL also randomizes spice when native assignment first reaches an
unassigned homeworld; an already assigned homeworld is left alone. It has
ESC/TAB/Spacebar communication-close and planner/hotbar keyboard handlers on
the game's central input callback. The user reported ESC/TAB did not close
dialogue and planner 1-4 did nothing in 0.5.13. Build 0.5.14 added diagnostics
and relaxed an over-strict UI-container enabled check. Build 0.5.15 added
Spacebar. Build 0.5.16's synthetic Goodbye click logged a zero command ID.
The 0.5.17 crash dump shows an invalid AddRef from reading the current event at
manager offset `0x1C`; the native event slot is at `0x20`. Build 0.5.18 reads
that slot, queues the action until the input callback returns, and rechecks the
active event and Goodbye button. The user confirmed that it hid the dialogue
but left Space controls locked and prevented reopening dialogue. Build 0.5.19
also passed the key through Spore's native input state machine before the
deferred action. The planet's "Speak with the colony" test still left the
communication event active after the window disappeared. Build 0.5.20 restored
control, but waited one second and made the dialogue button flash. Build 0.5.21
consumes the close key and recovers as soon as the CommScreen root is hidden,
retrying for at most 250 ms if its close transition spans frames. In-game
testing confirmed Space closes the planet's "Speak with the colony" dialogue,
restores movement, zoom, and travel immediately, allows reopening, and leaves
the Speak button steady. ESC, TAB, End, and submenu behavior remain
unverified. The number-key Space hotbar path is also awaiting in-game
confirmation. The DLL speeds up native
communication opening, collects stored spice from player-controlled colonies
when the player arrives at their star on the galaxy map, and reapplies the
configured cargo stack limit to the live Space-stage inventory through
`SetMaxCargoAmount()`.

The colony planner can copy a layout to a validated persistent file and apply
it to the edited colony or nearby player colonies on the same planet. The
apply path preserves protected and already occupied mismatched structures,
updates matching nouns in place, fills empty slots, selects a random compatible
Sporepedia item when the palette has no selection, charges available funds,
and reports one Colonist badge-count increment for each newly created building
or turret. Crop Circles also queue a persistent, slow uplift for
eligible non-homeworld planets.

Runtime feature switches are read from the root `config.psd1` by
`build-runtime.ps1` and compiled into the DLL. See the root
[`Implemented Features.md`](../Implemented%20Features.md) for the full default
feature inventory and [`TODO.md`](../TODO.md) for required in-game validation.

It intentionally does not cancel a disaster already in progress. The public
SDK does not expose a safe event-removal operation, and forcing a mission into
failed/completed state could trigger alerts, rewards, or relationship effects.

The project targets 32-bit Release with Visual Studio 2022's v143 toolset. It
uses the sparse SDK at `..\..\Tools\Spore-ModAPI-SDK`; no Visual Studio
extension, Ghidra installation, or full SDK checkout is required.
