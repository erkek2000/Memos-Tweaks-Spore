# Inputs and build provenance

Created 2026-09-08 from the user's local feature specification and game files.
Original Kisu mod and both feature documents were preserved.

| Input | Location / SHA-256 |
| --- | --- |
| Specification | `mod-features.md` (current), with `mod-features-old.md` retained as history |
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

`build.ps1` records pack output, verifies the compiled resource contents, builds
the optional archive, and records output hashes in `reports/`. The compiled
package is the standalone fork, not a patch that requires loading Kisu beside it.

Version 0.3.0's package-only feasibility audit also consulted VanillaCold's
public Solar Spore repository under `reference/VanillaCold_SolarSpore/`. It is
reference material only and is not packed into this mod. The audit confirmed
that Solar Spore's custom research interface is backed by a ModAPI C++ DLL;
no Solar Spore code or assets were copied into ERKEK2000 QoL.
