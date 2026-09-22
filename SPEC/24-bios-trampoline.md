# BIOS Trampoline (kernel/tramp.c / kernel/tramp.h)

**Files:** kernel/tramp.c (84 lines), kernel/tramp.h (55 lines)
**Status:** built by makefile's kernel/%.o pattern rule; part of `OBJS` (makefile:47); ABI slots 45-48

## Purpose

C-side marshaling for a PM→RM→BIOS→PM round trip: drop from 32-bit protected mode to 16-bit real mode, make one `int 10h` BIOS video call, and come back, all while the kernel keeps running. The actual mode switch is asm-only and lives in boot/stage2.s — "16-bit code cannot be linked at 1MB" (tramp.h:10) — this file's job is entirely to fill a shared register-block struct, invoke the switch, and read the answer back (tramp.c:1-2).

The strategic point (per project memory) is bigger than one BIOS call: it makes leaving VGA text mode a two-way door instead of a one-way one. `int 10h AX=0003` makes the BIOS rebuild font, palette, and CRTC timing from scratch — something no amount of direct register poking from protected mode can reconstruct (tramp.h:12-17). That's what makes `gfx_exit()` (below) a genuine, unconditional recovery path rather than a best-effort one.

This works *only* because the kernel runs 32-bit flat with no paging (tramp.h:7-9): there are no page tables to keep identity-mapped across the switch and no v86 monitor needed to trap GPFs — clearing PE and setting it back is the entire mechanism. Adding paging later means this file's entire approach needs re-deriving, not just tweaking (tramp.h:35-36 in the original memory note; see also the file's own framing at tramp.h:9).

## Public interface

ABI slots 45-48 (kernel/abi.h:107-110): `LW_SLOT_TRAMP_CALL`, `LW_SLOT_GFX_ENTER`, `LW_SLOT_GFX_EXIT`, `LW_SLOT_GFX_LIVE`. The slot comment (abi.h:102-106) flags the cost explicitly: "interrupts-off for the round trip, so not for anything in a loop," and singles out `GFX_EXIT` as "the one worth writing on paper" since it takes no arguments, tests no state, and therefore cannot fail to be callable — the textbook shape for an emergency-recovery ABI entry.

- `u16 tramp_call(u16 ax, u16 bx, u16 cx, u16 dx)` (tramp.c:23-34) — the general-purpose primitive: fills `TRAMP_REGS`, calls `tramp_int10()` (asm, boot/stage2.s), returns the ax the ROM left. es/di/bp/flags remain readable in `TRAMP_REGS` afterward for calls that answer with a pointer (tramp.h:37-39).
- `int gfx_enter(void)` (tramp.c:36-61) — switches to the VBE mode the boot harvest picked, with a linear framebuffer live. Returns 0 on success (framebuffer now at `BINFO->fb_phys`, `gfx_live()` reads 1), -1 if there's no usable candidate in the boot info block, -2 if the ROM refused the mode (tramp.h:41-45).
- `void gfx_exit(void)` (tramp.c:63-79) — back to 80×25 color text. No arguments, no precondition, no state test — safe to call even if no mode was ever set (tramp.h:47-49).
- `int gfx_live(void)` (tramp.c:81-84) — 1 between a successful `gfx_enter()` and the next `gfx_exit()` (tramp.h:51-52).

## Key structures / state

- `#define TRAMP_BLK_BASE 0xA000u`, `#define TRAMP_REGS_OFF 0x18u` (tramp.h:21-22) — the register block sits at 0xA018, mirroring the `TB_*` equs in boot/stage2.s.
- `tramp_regs_t` (tramp.h:26-28): `u16 ax, bx, cx, dx, es, di, bp, flags` — 8 fields, 16 bytes.
- 9 `_Static_assert`s in tramp.c (tramp.c:10-19) pin every field's offset (0x00 through 0x0E), the struct's total size (0x10), and the absolute address `TRAMP_BLK_BASE + TRAMP_REGS_OFF == 0xA018u` — the same offset-mirroring discipline used for `binfo_t` (spec/23-harvest-block.md) and `struct ata_dev` (spec/21-abi-table.md).
- `static int gfx_is_live` (tramp.c:21) — the only module-local state; backs `gfx_live()`.
- `TRAMP_REGS` macro (tramp.h:30) — `(volatile tramp_regs_t *)(TRAMP_BLK_BASE + TRAMP_REGS_OFF)`, the live view onto the shared block.

## Dependencies

- boot/stage2.s: owns `tramp_int10()` itself (the actual PM→RM→PE-clear→BIOS→PE-set→PM sequence) and the `TB_*` address constants this file's asserts pin against. This file cannot be understood in isolation from that asm.
- kernel/binfo.c / kernel/binfo.h: `gfx_enter()` reads `BINFO->flags` and `BINFO->vbe_mode` directly (tramp.c:41,46-50) — a second, independent validity check against the same struct that `binfo_valid()`/`binfo_dump()` also check (see spec/23-harvest-block.md's Messiness section: this duplication is flagged there too).
- kernel/console.h: `gfx_exit()` calls `con_reset_cursor()` (tramp.c:78), not `con_init()` — see Design notes below for why that distinction matters.
- kernel/abi.c: exposes all four functions as slots 45-48 and is the only place that references `gfx_enter`/`gfx_exit`/`gfx_live`/`tramp_call` by name outside this file and boot/stage2.s.

## Design notes worth keeping

- Four specific things the project memory calls out as "bite, and all four are handled — don't undo them," cross-checked against this file:
  1. Both PICs are fully masked for the round trip (stated in tramp.h:102's slot comment and the memory note; the actual masking is in boot/stage2.s, not visible in this C file) because the BIOS assumes PIC vectors at 0x08-0x0F while the kernel has remapped them to 0x20+.
  2. DATA16 descriptors must load before PE clears, or real mode inherits 4GB cached limits (unreal mode) — again the mechanism is in boot/stage2.s; this file's contract (`tramp_int10()` as an opaque call) is what makes that ordering invisible and safe from the C side.
  3. The kernel's stack lives above 1MB and is unreachable in real mode, so esp is parked at a fixed low address (`TB_ESP`) for the trip — not directly visible in tramp.c, but implied by the whole design; worth confirming against boot/stage2.s before assuming anything about C-level locals surviving the trip.
  4. VBE 4F02 needs `mode | 0x4000` for the linear-framebuffer bit — and this one *is* directly visible: `tramp_call(0x4F02, (u16)(mode | 0x4000), 0, 0)` (tramp.c:56), with an explicit comment that omitting bit 14 is "the standard way to end up in a banked mode holding a flat pointer that writes nowhere" (tramp.c:54-55).
- `gfx_exit()`'s deliberate unconditionality (tramp.c:69-71 comment: "no gfx_live() test, no input to wait on... A recovery path that depends on state or on a keypress is the one that deadlocks exactly when it is needed") is a real, generalizable design principle for any emergency-recovery function in this codebase, not just this one.
- `con_reset_cursor()` instead of `con_init()` after `gfx_exit()` (tramp.c:76-78) — documented reason: `con_init()`'s UART loopback self-test would discard any input already queued on COM1. This is exactly the kind of cross-file gotcha (discovered "while testing" per project memory) that's easy to silently regress if console.c is ever refactored without carrying the comment forward.
- `gfx_enter()`'s double-flag check (`BINFO_F_VBE | BINFO_F_LFB`, tramp.c:46-48) is explained inline: "without a linear framebuffer the fb_phys this makes live points at nothing a flat pointer can use" — worth keeping as a comment wherever this check is eventually re-homed.

## Messiness / review notes

- `gfx_enter()` re-derives boot-info validity (`binfo_valid()` + a specific flag mask) independently of `binfo_dump()`'s own flag-gated logic in kernel/binfo.c — flagged from both sides now (see spec/23-harvest-block.md too). A shared `binfo_gfx_ready()`-style accessor would remove one of the two places this check has to be kept in sync.
- `tramp_call()` zeroes `es`, `di`, `bp` unconditionally before every call (tramp.c:29-31) rather than accepting them as parameters — fine for the two calls this file actually makes (0x4F02 mode set, 0x0003 mode set to text), but any future caller needing es:di/bp as *input* (e.g. a BIOS call that takes a buffer pointer) will need a second entry point or a signature change, since the current `tramp_call(ax,bx,cx,dx)` has no way to set them.
- Return-value convention is inconsistent across the two `gfx_*` functions: `gfx_enter()` distinguishes "no candidate" (-1) from "ROM refused" (-2), while `gfx_exit()` has no return value at all (matching its "cannot fail" design) and `tramp_call()` just returns the raw ax. Not a bug, but a caller reading only the header has to remember three different error conventions in one small file.

## Open questions for the rewrite

- Should the boot-info validity check for graphics readiness be centralized in kernel/binfo.c as a named accessor, removing the duplicate check currently living in `gfx_enter()`?
- Is `tramp_call()`'s fixed four-register, zeroed-es/di/bp signature sufficient for every BIOS call this project will ever need, or does the rewrite want a version that accepts es/di/bp (and maybe a data pointer) as real parameters?
- Does the "no paging" precondition (tramp.h:9) get documented as a hard architectural constraint elsewhere (e.g. in whatever memory-management chapter follows), so a future paging feature doesn't silently break this file without anyone connecting the two?
- Is `gfx_is_live` worth promoting to something richer (e.g. tracking *which* mode is live, not just a boolean) now, before more BIOS-call wrappers are added that might want the same kind of state?
