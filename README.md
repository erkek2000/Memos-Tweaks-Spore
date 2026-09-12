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

Version **0.5.13** updates keyboard shortcuts to intercept the game's central
key-input callback rather than relying on a root-window event. ESC/TAB activate
the visible native Goodbye action, planner **1–4** select building palette
items, and number keys are routed to the visible native Space hotbar outside
the planner. Colony-pattern apply also avoids removing occupied live structures:
matching nouns can be updated, empty slots can be filled, and occupied
mismatched slots are preserved to prevent editor-reference crashes.

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
