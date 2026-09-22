# external/ — vendored & foreign reference material

**Files:**
- `ata.c` (481 lines), `ata.h` (95 lines)
- `i915_reg.h` (753 lines), `i915_reg_defs.h` (54 lines), `i915_reg(1).h` (7179 lines)
- `intel_display_regs.h` (3268 lines), `intel_display_reg_defs.h` (77 lines)
- `8101h_sector.bin` (512 B), `8101h_sector_1.bin.txt`, `8101h_sector_2.bin.txt` (hex dumps)
- `test.bin` (209 B), `test.bin.txt` (hex dump)

**Status:** NOT part of the make build. `grep -n "external" makefile` matches only the comment on line 75 ("Two **independ**ent flat binaries..." — a substring hit on the English word, not a path reference). No file under `external/` is named in `OBJS`, any compile rule, or any `#include` anywhere in `kernel/` or `boot/`.

## Purpose

A dumping ground for reference material collected while writing the real drivers: one prior ATA implementation this project's `kernel/ata.c` was ported from, a large vendored slice of the Linux kernel's i915/Pineview display register headers, and a couple of raw hex captures. Nothing here compiles into the image.

## Inventory

**`ata.c` / `ata.h`** — a complete, self-contained PIO ATA driver, structurally unrelated to `kernel/ata.c`'s API (`ata_device_t` vs `struct ata_dev`, `#include <driver/ata.h>` — a path that doesn't exist anywhere in this repo, so this file could not even compile here as-is). `kernel/ata.h` line 7 and `kernel/ata.c` line 1 both say "Ported from external/ata.c", so this is confirmed **origin material, not a duplicate build target** — keep it only as long as the port's provenance is worth tracing; nothing in the live kernel reads it. Verdict: **(b) dead reference**, safe to delete once you no longer need to diff the port against its source.

**`i915_reg.h`** (753 lines) — a hand-trimmed subset of Intel's register defs. Its own `#include "display/intel_display_reg_defs.h"` (line 29) points at a `display/` subdirectory that doesn't exist in `external/` — the file is already broken relative to its siblings, evidence it was copied in and not kept building. Verdict: **(b) dead, and already bit-rotted**.

**`i915_reg(1).h`** (7179 lines) — the *full* upstream Linux i915_reg.h, unrelated in scope to the 753-line file despite the near-identical name (`diff` confirms they differ throughout, not a byte-for-byte accidental double-paste — one is a curated excerpt, the other the entire upstream file). Pulls in `<drm/intel/pick.h>` and `<drm/intel/reg_bits.h>` (via `i915_reg_defs.h`), neither of which exists in this repo — not buildable here regardless. The `(1)` in the filename is the tell of a browser/tool "save as, name taken" duplicate-download artifact, not a deliberate second copy. Verdict: **(c) look like an accidental duplicate download of the *wrong* thing** — it isn't a duplicate of `external/i915_reg.h`'s *content*, but its *existence* (two files with the same stem, one clearly an accident of naming) is exactly the kind of clutter worth deleting. Keep at most one canonical upstream reference, not two.

**`intel_display_regs.h`** / **`intel_display_reg_defs.h`** — same story as the i915 pair: verbatim vendored Linux DRM headers (`SPDX-License-Identifier: MIT`, `Copyright © 2022/2025 Intel Corporation`), not written for this project, not buildable standalone (needs the rest of the Linux DRM tree). Verdict: **(b) dead vendored reference**.

**`8101h_sector.bin` + the two `.txt` hex dumps** — a raw 512-byte sector capture (filename suggests a Realtek RTL8101H NIC-related probe) with two slightly different hex-dump renderings. No source anywhere reads these; they look like scratch captured while poking hardware from the monitor. Verdict: **(b) dead scratch data** — keep only if the capture itself (the actual byte values) is still needed as a reference; the sector filename gives no other project file any reason to depend on it.

**`test.bin` + `test.bin.txt`** — another small raw capture / hex dump, same character as the 8101h pair, no name tying it to any current subsystem. Verdict: **(b) dead scratch data**.

## Design notes worth keeping

None of this directory's *code* is worth preserving as code — it's all either superseded (ata.c, already ported and improved in `kernel/ata.c` per that file's own header comment) or unbuildable-as-is vendor dumps. The only thing with residual value is provenance: if a future display driver needs a real register name, `i915_reg(1).h`/`intel_display_regs.h` are the correct upstream source to re-derive constants from — but that's a "consult once, then discard" relationship, not a reason to keep 10,000+ lines checked in.

## Messiness / review notes

- Two files sharing the stem `i915_reg` (`i915_reg.h` and `i915_reg(1).h`) with wildly different sizes and one already referencing a nonexistent subdirectory (line 29 of the smaller one) is the single clearest piece of accidental clutter in the whole tree — the kind of thing that makes a directory listing actively misleading (which one is "the" i915_reg.h?).
- `external/ata.c`'s own `#include <driver/ata.h>` never matched this repo's layout at any point — it was never buildable here, meaning it was dropped in purely as prose reference from day one, not as a false-started build target.
- The two hex-dump/bin pairs (`8101h_sector*`, `test.bin*`) have no comment, README, or commit message context in the directory itself explaining what they were for — their meaning is already at risk of being lost.
- Total footprint: ~11,600 lines / ~490KB of material, none of it built, none of it referenced by a single line of `kernel/` or `boot/`.

## Open questions for the rewrite

- **Delete outright:** `i915_reg(1).h` (redundant/broken-path duplicate stem), `8101h_sector*.bin(.txt)`, `test.bin(.txt)` — no code anywhere references these, no comment explains their continued relevance.
- **Delete after a final diff pass:** `external/ata.c`/`ata.h`, once you've confirmed `kernel/ata.c`'s own header comment (which already documents "ported from, two things differ") captures everything worth remembering about the port.
- **Keep exactly one copy, outside the source tree if possible:** the upstream Intel display register headers (`i915_reg.h`, `intel_display_regs.h`, `intel_display_reg_defs.h`, `i915_reg_defs.h`) — useful as a lookup reference when the GFX half of the harvest block (see `spec/23-harvest-block.md`) finally gets a real driver, but they don't need to live inside a source tree that's about to be restyled; a bookmark to the upstream Linux source would serve the same purpose with zero maintenance cost.
- If any of these files' knowledge *is* still load-bearing (e.g. a specific register offset already proven correct against real Pineview hardware), pull just that value out into a comment in the eventual gfx driver before deleting the vendored header it came from — don't let the fact die with the file.
