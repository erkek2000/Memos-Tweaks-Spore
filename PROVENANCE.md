# Inputs and build provenance

Created 2026-09-08 from the user's local feature specification and game files.
The feature-request scope and current decisions are consolidated in
[`Implemented Features.md`](Implemented%20Features.md) and [`TODO.md`](TODO.md);
the former duplicate feature, hold, issue, and implementation summaries were
absorbed during the documentation cleanup.

| Input | Location / SHA-256 |
| --- | --- |
| Specification | User's original local feature request; current coverage is indexed in `Implemented Features.md` and `TODO.md` |
| Kisu 1.5 | `../KisuTweaks-1.5/KisuTweaks-1.5/KisuTweaks.package` |
| Kisu package SHA-256 | `366C74E145B6938211177D90FA33F91DB79D0E8179C33E72A08349040826F6E6` |
| SMFX 2.2.27 | `../Tools/SporeModderFX/SporeModderFX.jar` |
| SMFX JAR SHA-256 | `E991CDE3134937BF46C1D20CF7FCCC5DCCAE710B73837DFAD5A31ACEA7FD36C8` |

Vanilla reference inputs, read-only from the installed Steam Spore directory:

- `Data/Spore_Game.package`
- `Data/PatchData.package`
- `DataEP1/Spore_EP1_Data.package`

`tools/InspectPackages.java` uses the existing SporeModder FX DBPF and property
libraries to selectively extract matching Kisu properties and related tuning,
space-event, and tool resources. It writes decoded references only beneath the
supplied output directory. `reference/vanilla/<package name>/` records which
archive supplied each reference. PatchData supplies the restored values; base
game values agree, and GA also confirms the 70/100 core travel radii.

The Kisu reference was unpacked with SMFX's `PropConverter` only, leaving other
asset formats intact. `project/` was copied from that reference excluding 110
tiered badge overrides, then edited. The old `sporemaster/names.txt` copy was
omitted from the editable project because the SMFX packer generates that
dictionary itself; including both caused a duplicate metadata resource ID.
The original dictionary remains available in `reference/kisu/`.

`build.ps1` records pack output, verifies the compiled resource contents, and
records the package hash in `reports/`. `build-runtime.ps1` then builds the DLL,
creates the complete installer archive, and records all release hashes. The compiled
data package is the standalone Kisu fork, not a patch that requires loading
Kisu beside it. Runtime 0.1.0 pairs it with a ModAPI DLL, so the runtime edition
itself must be installed and launched through ModAPI.

Version 0.3.0's package-only feasibility audit also consulted VanillaCold's
public Solar Spore repository under `reference/VanillaCold_SolarSpore/`. It is
reference material only and is not packed into this mod. The audit confirmed
that Solar Spore's custom research interface is backed by a ModAPI C++ DLL;
no Solar Spore code or assets were copied into ERKEK2000 QoL.

Runtime 0.1.0 uses a sparse checkout of the official Spore ModAPI SDK at
`../Tools/Spore-ModAPI-SDK/`. Only the SDK, Detours, EASTL dependencies, and a
small official example were checked out. The Bio Protector detour was written
for this project; the example supplied the official project/build pattern.
