# ERKEK2000 QoL Runtime

A standalone Kisu 1.5 package with an optional ModAPI runtime DLL for Galactic
Adventures. The default build retains Kisu's 110 badge-tier overrides and
selected gameplay features, restores several vanilla balance values, and adds
runtime quality-of-life tools.

## Start here

- [`Implemented Features.md`](Implemented%20Features.md) is the full inventory
  of shipped behavior, inherited Kisu content, default values, and build-time
  switches.
- [`TODO.md`](TODO.md) is the single queue for incomplete features, unresolved
  decisions, and required game tests.
- [`TESTING.md`](TESTING.md) has detailed play-test procedures and static build
  verification results.
- [`REVERSE_ENGINEERING.md`](REVERSE_ENGINEERING.md) contains SDK findings,
  crash analyses, planner internals, and system-valuation research.
- [`PROVENANCE.md`](PROVENANCE.md) records source inputs, hashes, and build
  lineage.

## Current build

Version **0.5.13** was reported to have non-working ESC/TAB dialogue closing
and planner 1–4 shortcuts. **0.5.14** added diagnostics and relaxed an
over-strict UI-container enabled check. **0.5.15** added unmodified Spacebar.
The synthetic Goodbye click in **0.5.16** still reported command ID `00000000`
and did not close the dialogue. **0.5.17** crashed while reading the current
event from the wrong manager offset. **0.5.18** reads the game's event slot at
offset `0x20` and queues the exit action until the input callback returns,
revalidating the active event and Goodbye button before dispatch. The user
confirmed this hid the planet's "Speak with the colony" dialogue but left Space
controls locked and prevented reopening it. **0.5.19** also passed the close
key through Spore's native input state machine, but the user's log still showed
the communication event active after the window disappeared. **0.5.20** fixed
the stuck state, but waited one second and made the planet's dialogue button
flash. **0.5.21** consumes the shortcut key and recovers as soon as the
CommScreen root is hidden, retrying for at most 250 ms if the close transition
spans frames. In-game testing confirmed Space closes the planet's "Speak with
the colony" dialogue, restores movement, zoom, and travel immediately, allows
dialogue reopening, and leaves the Speak button steady. ESC, TAB, End, and
submenu behavior still need testing. Keyboard-hook attachment and key dispatch are logged to
`%APPDATA%\Spore\ERKEK2000_QoL\keyboard-input.log`. Do not treat the dialogue,
planner, or Space hotbar controls as working beyond the tested Spacebar path
until the remaining checks in `TODO.md` pass. Colony-pattern apply avoids
removing occupied live structures: matching nouns can be updated, empty slots
can be filled, and occupied mismatched slots are preserved to prevent
editor-reference crashes.

The installable artifact is
[`dist/ERKEK2000_QoL_Runtime.sporemod`](dist/ERKEK2000_QoL_Runtime.sporemod).
It contains the compiled package, 32-bit runtime DLL, and Galactic Adventures
manifest. The most recent package verification retained all 477 Kisu resource
IDs and all 110 badge overrides. Static checks and compilation pass; required
in-game validation is still listed in `TODO.md`.

Install through the Spore ModAPI Easy Installer and launch Galactic Adventures
with the ModAPI Launcher Kit. Do not install this alongside `KisuTweaks.package`,
`NambuCargoStack999.package`, or another ERKEK2000 QoL edition; this package
replaces Kisu's package. Back up `%APPDATA%\Spore` and test with a disposable
galaxy first. Existing galaxies do not regenerate star placement, undo earned
badges, or refund prior purchases.

## Build

From this directory, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-runtime.ps1
```

Edit [`config.psd1`](config.psd1) before building to change package values or
compiled runtime switches. Settings are build-time only; the game does not read
the configuration file. The build stages a temporary project, packs it with
SporeModder FX, verifies the actual package, compiles the runtime DLL, and
creates installer archives beneath `dist/`. Build reports are written under
`reports/`.

To build the included Kisu-baseline verification preset:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-runtime.ps1 `
  -ConfigurationFile .\configs\KisuBaseline.psd1
```

That preset is for verification; normal release builds use `config.psd1`.
`runtime/README.md` describes the DLL build component. The untouched decoded
Kisu baseline and selected vanilla references are under `reference/` and are
not packed into the mod.
