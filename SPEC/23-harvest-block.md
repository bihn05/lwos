# Boot Info / Harvest Block (kernel/binfo.c / kernel/binfo.h)

**Files:** kernel/binfo.c (127 lines), kernel/binfo.h (91 lines)
**Status:** built by makefile's kernel/%.o pattern rule; part of `OBJS` (makefile:47); ABI slots 27-28

## Purpose

Reader side of a fixed-format data block at physical 0x9000 that the real-mode stub fills in before protected mode starts, and that the kernel (and hand-typed code) only ever reads. The design principle stated in the header comment (binfo.h:5-7) is "dig out what is hard to get while it is still easy to get, and park it somewhere safe" — some hardware facts (VBE mode info, linear framebuffer address, EDID) are only cheaply reachable via real-mode BIOS calls (int 10h), so the stub gathers them once, during the narrow window before the BIOS interface goes away, and leaves them for everything downstream. This file is entirely passive: `binfo.c` "Nothing here writes to 0x9000" (binfo.c:2).

The inclusion test for a field is explicit (binfo.h:9-13): does getting it need real mode, or a destructive probe only safe before a driver owns the device? VBE/EDID qualify; a PCI BAR does not, since `pci_read32` works at any time — the PCI-shaped fields in the block (gfx_bdf, gfx_ids, gfx_irq, gfx_bars, gfx_sizes) are explicitly a convenience snapshot, not a dependency other code should require.

## Public interface

ABI slots 27-28 (kernel/abi.h:67-68): `LW_SLOT_BINFO_VALID` → `int binfo_valid(void)`, `LW_SLOT_BINFO_DUMP` → `void binfo_dump(void)`. Also reachable from the kernel monitor as the `xb` command (per project memory).

- `int binfo_valid(void)` (binfo.c:30-33) — true iff `BINFO->magic == BINFO_MAGIC`. This is the only correctness gate; every other reader is expected to call it first.
- `void binfo_dump(void)` (binfo.c:35-127) — prints the whole block: header (address, size, flags), VBE mode/framebuffer geometry and RGB mask layout if `BINFO_F_VBE`, GFX PCI snapshot if `BINFO_F_GFX`, EDID presence if `BINFO_F_EDID`.

## Key structures / state

- `#define BINFO_BASE 0x9000u`, `#define BINFO_MAGIC 0x4F464E42u` ('BNFO') (binfo.h:25-26).
- `BINFO_O_*` offset macros (binfo.h:29-52) — the assembly-facing layout, frozen and append-only per the header comment (binfo.h:21-23: "Fields may only be appended, and `size` says how much of the block a given build actually filled").
- `binfo_t` struct (binfo.h:67-82) — the C mirror of the same layout, including explicit `_pad0`/`_pad1`/`_pad2` padding fields to keep it byte-exact with the asm offsets.
- 18 `_Static_assert`s in binfo.c (binfo.c:10-28) pin every struct field's offset against its `BINFO_O_*` macro, plus `sizeof(binfo_t) == BINFO_O_END`. Same pattern as kernel/abi.c's ata_dev asserts and kernel/tramp.c's register-block asserts — this project's standard technique for keeping an asm-visible layout and its C mirror honest.
- Flag bits (binfo.h:58-63): `BINFO_F_VBE` (0x01, VBE fields trustworthy), `BINFO_F_LFB` (0x02, that mode has a linear framebuffer), `BINFO_F_EDID` (0x04), `BINFO_F_GFX` (0x08, a class-03 PCI function was found), `BINFO_F_QUIET` (0x10, GPU interrupts were masked before handoff), `BINFO_F_MODESET` (0x20, the mode is actually live, not just picked). `F_VBE` vs `F_MODESET` are deliberately separate (binfo.h:54-57): the harvest runs while the console is still writing to 0xB8000 text memory, so it can pick and validate a mode without daring to switch to it and blank the only output.
- Address rationale: 0x8000 is the E820/ARDS buffer (see spec/22-boot-memory-layout.md) running to roughly 0x8A0C, so this block starts at 0x9000, above the MBR's constrained low region rather than squeezed below it (binfo.h:15-19).

## Dependencies

- The real-mode stub in boot/stage2.s is the sole writer — this chapter's files never populate the block, only validate and print it. That write-side code (`vbe_harvest` per project memory) isn't in this review's file set.
- kernel/abi.c exposes `binfo_valid`/`binfo_dump` as slots 27-28 and also stores `BINFO_BASE` itself at slot 2 (`LW_SLOT_BOOTINFO`) as a raw address, not a function pointer — so hand-typed code has two independent ways to find the block: read slot 2, or just know the constant 0x9000.
- kernel/tramp.c's `gfx_enter()` reads `BINFO->flags`/`BINFO->vbe_mode` directly (not through this file's functions) to decide whether a VBE mode switch is possible — see spec/24-bios-trampoline.md. That's a second, independent consumer of the same struct with its own validity check duplicated inline (checks `BINFO_F_VBE | BINFO_F_LFB` rather than calling `binfo_valid()` first).
- kernel/console.h — used for every print call in `binfo_dump()`.

## Design notes worth keeping

- The magic-first validation pattern (`binfo_valid()` gating everything else) is simple and consistently applied — worth keeping regardless of what replaces the block's contents.
- Per project memory: the GFX half of the stub-side harvest (`BINFO_F_GFX`/`BINFO_F_QUIET`) is not yet written — the reader code in `binfo_dump()` (binfo.c:94-120) already handles it correctly and will simply stay silent (the `if (f & BINFO_F_GFX)` block never executes) until the stub sets those bits. This is real, working forward-compatible reader code sitting ahead of its writer, not dead code — don't delete it during a simplification pass without checking whether the stub-side work is still planned.
- `binfo_dump()`'s nested flag checks mirror the struct's own "narrow inclusion test" philosophy: each printed section is gated on the flag that specifically means "these bytes are trustworthy," not just on `binfo_valid()` generally (e.g. LFB fields only print under `f & BINFO_F_LFB`, binfo.c:71-89).
- The append-only/frozen-offset discipline here is the same one governing kernel/abi.h's slot table (spec/21-abi-table.md) — this project has one consistent answer to "how do we let a fixed memory layout evolve safely," applied independently in at least three places (ABI slots, binfo offsets, tramp register block).

## Messiness / review notes

- Two independent, uncoordinated readers of the same struct: `binfo_dump()` here checks `binfo_valid()` first and then per-field flags; `gfx_enter()` in kernel/tramp.c (tramp.c:41-52) checks `binfo_valid()` and then a *different*, narrower flag combination (`VBE|LFB` specifically) inline, duplicating the validity-then-flag-check pattern rather than sharing a helper. Not wrong, but if a third consumer appears it's worth factoring a shared accessor rather than re-deriving the check a third time.
- `binfo_dump()` is one 92-line function handling four independent sections (header, VBE/framebuffer, GFX/PCI snapshot, EDID) via nested `if`s — readable today at this size, but it's the kind of function that grows one more `if (f & BINFO_F_X)` block per future harvest field with no structural push-back. Worth watching if the block keeps growing.
- No length/bounds concern here (binfo.c writes nothing), but note that `binfo_t`'s `edid[128]` (binfo.h:80) is read raw with no accessor beyond "print that it's present" (binfo.c:122-126) — actual EDID parsing, if ever needed, has to happen elsewhere against the raw bytes at `BINFO_BASE + BINFO_O_EDID`.

## Open questions for the rewrite

- Should `gfx_enter()`'s inline validity/flag check move into a shared `binfo_*` accessor (e.g. `binfo_gfx_ready()`) instead of duplicating the `binfo_valid()`-then-flags pattern a second time?
- Is the 0x9000 fixed address still the right home once/if paging or a real memory allocator exists, or does this block eventually want to be copied into `.bss` the way the E820 map is (spec/22-boot-memory-layout.md)? Right now it's read directly from low memory on every call, unlike the memory map which gets copied up to 1MB.
- Does the GFX harvest (`BINFO_F_GFX`/`BINFO_F_QUIET`, stub-side asm) still belong on the roadmap, and if so is it still meant to stay hand-written asm as a practice exercise, or does the rewrite want it in C?
- Is `binfo_dump()`'s single large function worth splitting into one print-helper per section now, ahead of further growth, or is that premature for a debug-dump function?
