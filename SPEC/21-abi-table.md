# ABI Table (kernel/abi.c / kernel/abi.h)

**Files:** kernel/abi.c (200 lines), kernel/abi.h (180 lines)
**Status:** built by makefile's kernel/%.o pattern rule; `kernel/abi.o` is deliberately first in `OBJS` (makefile:46) so `.abi` lands first inside `.kernel`, matching link.ld's ASSERT (link.ld:85-86)

## Purpose

The user hand-types 32-bit assembly directly into the running debuggee on real hardware and writes call addresses down on paper (per project memory). A raw function address is useless for that workflow because every rebuild moves it. `kernel/abi.c` is the fix: a `const void *` array placed in its own `.abi` linker section, which link.ld pins at exactly 0x100000. Slot *n* is forever at `0x100000 + 4*n`; the linker fills in whatever the current build's address for that function is. The table needs no runtime initialization — because it's a linker-resolved const array, it's valid the instant the kernel copy to 1MB finishes, before `kmain` even runs (abi.c:1-7).

## Public interface

Calling convention (abi.h:11-19): cdecl, arguments pushed right-to-left, caller cleans the stack, return in eax, ebx/esi/edi/ebp preserved, eax/ecx/edx clobbered. DS/ES must hold the flat data selector; esp must point at real stack. Called as `call [0x100000+4*slot]` or loaded into a register first.

Full slot table (abi.h:32-144), 61 slots (0-60, `LW_SLOT_COUNT` = 61) grouped as:
- 0-3: header — `MAGIC` ('LWAB'), `VERINFO` (nslots<<16|version), `BOOTINFO` (phys addr of the block in kernel/binfo.h, or 0), one reserved slot
- 4-17: console (con_init/putc/puts/puts_pad/hex8/16/32/64/dec32/getc/getkey, serial ready/present/getc)
- 18-26: pci (read32/16/8, write32, present, scan, detail, class_name, vendor_name)
- 27-28: boot info block reader (binfo_valid, binfo_dump)
- 29-35: keyboard (probe, enable, raw, feed, poll, mods, reset_state)
- 36-42: ata (init, detect, count, get, read, write, strerror)
- 43-44: scratch buffer — not functions; slot 43 is the buffer's address, slot 44 its size
- 45-48: BIOS trampoline (tramp_call, gfx_enter, gfx_exit, gfx_live) — see spec/24-bios-trampoline.md
- 49-51: boot handoff — not functions except KMAIN_ENTRY; `__bss_start`, `__stack_top`, `kmain` so the stub can load a kernel it knows nothing about
- 52-60: fs — FAT32 (fs_init/open/read/size/close/create/replace/strerror/dump) — see spec/25 or wherever fs.c's chapter lands

`abi_dump()` (abi.c:186-200) prints slot/address/target/name for every entry — this is what the monitor's `x` command and `make map` both surface.

## Key structures / state

- `lw_abi[]` (abi.c:47-118) — the actual table, `__attribute__((section(".abi"), used, aligned(16)))`.
- `abi_names[]` (abi.c:122-184) — a parallel `static const char *const` array in `.rodata`, deliberately *not* in `.abi` (abi.c:120-121: "the slot table must stay a dense array of addresses so slot n is at BASE+4n with nothing between").
- `lw_scratch[LW_SCRATCH_SIZE]` (abi.c:38, abi.h:172-173) — a 4KB `.bss` buffer backing slots 43/44. It's in `.bss` specifically because stage2 zeroes BSS, so it's the one block of RAM whose contents are guaranteed known at boot (abi.c:36-37) — unlike arbitrary hand-picked addresses, which on real hardware hold BIOS/previous-OS leftovers.
- `ATA_DEV_O_*` offset macros (abi.h:152-168) — a hand-maintained mirror of `struct ata_dev`'s layout for slot 39's caller, since hand-typed asm has no struct definition to reach into. Twelve `_Static_assert`s in abi.c (abi.c:21-32) pin every field offset plus the struct's total size (`ATA_DEV_SIZEOF` = 88) against the real struct in kernel/ata.h.
- `LW_ABI_MAGIC` = 0x4241574C ('LWAB'), `LW_ABI_VERSION` = 1 (abi.h:28-29).

## Dependencies

Binds together nearly the whole kernel: abi.c `#include`s console.h, pci.h, binfo.h, kbd.h, ata.h, tramp.h, fs.h (abi.c:10-16) purely to populate the table — it is the one file that must know about every subsystem. link.ld depends on `.abi` being emitted first (its own ASSERT enforces this). boot/stage2.s reads slots 49-51 (`__bss_start`, `__stack_top`, `kmain`) to load and jump into a kernel it has no other knowledge of (abi.h:112-120) — this is the one place the boot stub and the kernel's build-time layout are coupled through the table rather than through link.ld symbols directly.

## Design notes worth keeping

- "THE ONE RULE: slots are append-only. Never renumber, never reuse, never remove." (abi.h:21-23) — this is the load-bearing constraint of the entire file; every other design choice here serves it.
- Retiring a slot means pointing it at a stub, not deleting it (abi.h:22) — preserves the paper addresses.
- The `_Static_assert` pattern for `struct ata_dev` (abi.c:18-32) is a good template: whenever a struct's raw layout is exposed to hand-typed asm via offset macros, pin every field with a static assert so a reorder is a build failure, not a silent wrong-field read. The same pattern is used for `binfo_t` in binfo.c and `tramp_regs_t` in tramp.c.
- Slot comments in abi.h consistently give the address (`/* 0x100010 ... */`) next to the enum value — this is what makes the header usable as the actual paper reference alongside `make map`.
- ATA_DEV_O_BACKEND's placement note (abi.h:162-167): appended after `firmware` deliberately, landing in trailing padding, so no earlier offset moves and `ATA_DEV_SIZEOF` doesn't grow — a concrete example of the append-only discipline applied to a struct, not just the slot table.

## Messiness / review notes

- The header comment "mmap next" at abi.h:142 is a leftover TODO/note-to-self inside a doc comment meant to be a stable reference — a small thing, but it's the kind of authoring residue that accumulates when a header is both spec and scratch pad.
- `LW_SLOT_RESERVED3` (abi.h:37, abi.c:51,126) exists with no stated purpose beyond "reserved" — worth deciding in the rewrite whether reserved padding slots are policy (leave gaps for alignment/future grouping) or accidental (a slot 0-3 header block that just happened to need a 4th filler).
- Table population in abi.c (abi.c:47-118) and the name table (abi.c:122-184) are two independently-maintained arrays covering the same 61 entries; nothing enforces they describe the same slot with the same intent beyond both being indexed by the same enum — a copy-paste slip (wrong name next to a slot) would compile fine and only show up as a cosmetic bug in `abi_dump()`/`make map`.
- The file mixes true function pointers, raw data addresses (scratch, bootinfo), and linker symbols (`bss_start`, `stack_top`) in one array typed `void *const lw_abi[]` — semantically three different kinds of slot sharing one C type. Not wrong (the whole point is "one dense array of pointer-sized values"), but worth flagging since a caller has to know out-of-band which kind slot *n* is; `abi_names[]`'s parenthesized entries like `"(bootinfo)"` are the only in-band hint (abi.c:125,166-167,172-174).

## Open questions for the rewrite

- Is the append-only slot table still the right mechanism if the "hand-type asm on paper" workflow changes shape, or is it worth keeping unconditionally as a stable ABI boundary regardless of how it's used?
- Should the three slot *kinds* (function pointer / data address / linker symbol) be split into separate tables, or is one dense array still preferred for the `call [0x100000+4*slot]` ergonomics?
- Does `LW_SLOT_RESERVED3` get a real purpose, get documented as deliberate padding, or get retired like any other slot would be?
- Is the `_Static_assert`-mirrored-offset-macro pattern (abi.h + abi.c, also used for binfo_t and tramp_regs_t) worth generalizing into one convention/macro, given it's now used three times independently?
- Should `abi_names[]` be generated from the same source of truth as `lw_abi[]` (e.g. an X-macro) to remove the two-array duplication risk?
