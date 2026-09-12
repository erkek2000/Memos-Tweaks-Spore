# Runtime component

`ERKEK2000_QoL_Runtime.dll` intercepts creation of the
`biospherecollapse` event. If the proposed source planet's saved
`mPlanetObjects` list contains the `SG_colonytool_bioprotector` model, mission
creation returns null and the future eco-disaster is suppressed.

The DLL also randomizes spice when native assignment first reaches an
unassigned homeworld; an already assigned homeworld is left alone. It installs
guarded ESC/TAB communication closing, speeds up the native communication
opening, and collects stored spice from player-controlled colonies when the
player arrives at their star on the galaxy map. It reapplies the configured
cargo stack limit to the live Space-stage inventory through
`SetMaxCargoAmount()`. A detour on the game's central keyboard-input callback
maps 1/2/3/4 to visible colony-building palette items, routes number keys to the
visible native Space tool hotbar outside the colony planner, and maps ESC/TAB
to the visible native communication Goodbye action. This handles keys even
when focus is on a child UI control; submenu confirmation behavior remains
native when Goodbye is hidden or disabled.

The colony planner can copy a layout to a validated persistent file and apply
it to the edited colony or nearby player colonies on the same planet. The
apply path preserves protected and already occupied mismatched structures,
updates matching nouns in place, fills empty slots, selects a random compatible
Sporepedia item when the palette has no selection, charges available funds,
and reports progress to the Civs Promoted badge for each newly created
building or turret. Crop Circles also queue a persistent, slow uplift for
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
