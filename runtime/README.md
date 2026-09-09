# Runtime component

`ERKEK2000_QoL_Runtime.dll` intercepts creation of the
`biospherecollapse` event. If the proposed source planet's saved
`mPlanetObjects` list contains the `SG_colonytool_bioprotector` model, mission
creation returns null and the future eco-disaster is suppressed.

The same DLL installs a guarded root-window keyboard procedure for ESC/TAB
communication closing and an arrival-edge galaxy-map collector for stored
spice on player-controlled colonies. It also reapplies the configured cargo
stack limit to the live Space-stage inventory through `SetMaxCargoAmount()` and
maps 1/2/3/4 to the visible House/Entertainment/Factory/Turret palette items
while the colony editor is active. Outside that editor, it redispatches
unmodified number keys to the currently visible native Space tool panel so the
existing per-tab hotbar can select its own slots.
Runtime feature switches are read from the root `config.psd1` during
`build-runtime.ps1` and compiled into the DLL.

It intentionally does not cancel a disaster already in progress. The public
SDK does not expose a safe event-removal operation, and forcing a mission into
failed/completed state could trigger alerts, rewards, or relationship effects.

The project targets 32-bit Release with Visual Studio 2022's v143 toolset. It
uses the sparse SDK at `..\..\Tools\Spore-ModAPI-SDK`; no Visual Studio
extension, Ghidra installation, or full SDK checkout is required.
