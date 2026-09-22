# Boot Memory Map (kernel/mmap.c / kernel/mmap.h)

**Files:** kernel/mmap.c (148 lines), kernel/mmap.h (45 lines)
**Status:** built by makefile's kernel/%.o pattern rule; part of `OBJS` (makefile:47)

## Purpose

Reader/formatter for the BIOS memory map (E820/ARDS records: base, length, type) that stage2 collects in real mode before switching to protected mode. This file itself does no BIOS calls — it only copies the raw table out of low memory into `.bss` at 1MB (where it survives long-term) and offers query/print helpers over it (mmap.c:1-5).

The reason this copy step exists at all: the E820 buffer lives at a fixed low address (0x8000) that is "fair game for reuse later" (mmap.h:31) by anything else that wants conventional memory, and the kernel's own `.bss` doesn't exist yet at the moment stage2 collects the map — BSS is cleared only after the protected-mode switch, and lives at 1MB regardless (mmap.h:8-10).

## Public interface

Not exposed through the ABI table directly by this file (no `mmap_*` entries appear in kernel/abi.c's slot list) — the memory map is consumed internally, likely by kernel/main.c and/or an allocator. Functions (mmap.h:32-43):
- `void mmap_init(void)` — copies the table out of the low buffer; idempotent, safe to call once, later calls are no-ops (mmap.c:21-42, `inited` guard at mmap.c:12,27-29)
- `void mmap_dump(void)` — prints every entry plus usable total and the largest range ≥1MB (mmap.c:101-148)
- `u32 mmap_count(void)` — entry count, 0 if stage2 found nothing
- `const struct ards *mmap_entry(u32 i)` — bounds-checked single entry (mmap.c:16-19)
- `u64 mmap_usable(void)` — total bytes of type-1 (usable) memory (mmap.c:44-52)
- `int mmap_largest(u64 *base, u64 *len)` — largest usable range at or above 1MB, i.e. the one actually worth allocating from (mmap.c:54-71)

## Key structures / state

- `#define MMAP_BUF 0x8000` (mmap.h:12) — where stage2 left the raw header+entries.
- `#define MMAP_MAGIC 0x53445241u` ('ARDS') (mmap.h:13) — sanity check that stage2 actually ran the collection and found something (mmap.c:31).
- `struct ards { u64 base; u64 len; u32 type; } __attribute__((packed))` (mmap.h:24-28) — one 20-byte ARDS record, matches the raw BIOS int 15h/E820 format directly.
- `#define MMAP_MAX 128` (mmap.h:14) — fixed-size static table `tab[MMAP_MAX]` (mmap.c:9); entries beyond this are silently dropped (mmap.c:36-37, `if (count > MMAP_MAX) count = MMAP_MAX;`).
- ARDS type constants (mmap.h:18-22): `ARDS_USABLE=1`, `ARDS_RESERVED=2`, `ARDS_ACPI_REC=3`, `ARDS_ACPI_NVS=4`, `ARDS_BAD=5`.
- `MMAP_F_E801` flag (mmap.h:15) — set when the map was "synthesised from int 15h AX=E801h" rather than genuine E820 records; `mmap_dump()` reports the ranges as approximate in that case (mmap.c:114-115).
- Module-local state: `tab[]`, `n_entries`, `flags`, `inited` (mmap.c:9-12) — all file-scope statics, no reentrancy considerations (fine for a single-core boot-time kernel).

## Dependencies

- **boot/stage2.s** is the producer: it must write the 4-byte magic + count + flags header followed by ARDS entries starting at `MMAP_BUF+16` (mmap.c:23-24, header layout implied: `hdr[0]`=magic, `hdr[1]`=count, `hdr[2]`=flags) before protected mode starts. This file only ever *reads* that layout; the producing code isn't in this review's file set.
- kernel/console.h — used for all the `con_*` print calls in `mmap_dump()`.
- Project memory (boot-memory-layout) notes 0x8000 runs to ~0x8A0C in practice and that A20 must be enabled before the eventual copy to 1MB or the copy wraps onto 0x000000 and destroys the IVT — that A20 concern belongs to stage2, not this file, but explains *why* `mmap_init()`'s copy into `.bss` at 1MB is safe to trust only once A20 is confirmed live.
- Nothing in the reviewed kernel/*.c set appears to call `mmap_init()` or the query functions — likely called from kernel/main.c (not covered by this chapter) and/or a physical allocator that doesn't yet exist in this tree.

## Design notes worth keeping

- `print_size()` (mmap.c:87-99) — right-shifts by 10 in a loop while the value is both ≥1024 and a whole multiple of 1024, so round sizes print as "32M" rather than a raw byte count, but a non-round size prints in whatever unit it stopped at rather than silently truncating. Comment at mmap.c:96 notes the u32 cast is safe because real memory maps never approach 2^54 bytes.
- `mmap_largest()`'s restriction to ranges "at or above 1MB" (mmap.c:59, `tab[i].base < 0x100000` excluded) encodes a real policy: the kernel itself lives at 1MB, so anything below it is not free to hand out as general-purpose usable memory regardless of what the BIOS calls it.
- The `inited` guard (mmap.c:12,27-29) protecting `mmap_init()` is a simple, correct idempotency pattern worth keeping regardless of what replaces the surrounding code.
- End-address printing in `mmap_dump()` (mmap.c:127-129) explicitly computes `base+len-1` and notes why: "end address is inclusive; a zero-length entry cannot appear, stage2 drops those" — a small but easy-to-get-wrong off-by-one that's documented rather than silently correct.

## Messiness / review notes

- No `mmap_*` slots exist in the ABI table (kernel/abi.c), unlike almost every other kernel subsystem reviewed so far (console, pci, binfo, kbd, ata, tramp, fs all have slots). Either this is intentionally internal-only, or it's a gap — hand-typed asm currently has no way to query the memory map through the documented ABI mechanism, only kernel-internal callers (if any exist) can reach it.
- `mmap_dump()`'s "no BIOS memory map" message (mmap.c:106-108) is printed only when `n_entries == 0`, but nothing distinguishes "stage2 never ran E820 at all" from "stage2 ran it and got zero usable entries" — both produce the same message. Minor, but worth a flag bit if that distinction ever matters.
- `flags` is a module-global set only inside `mmap_init()` from `hdr[2]` (mmap.c:35) and read only inside `mmap_dump()` (mmap.c:114) — its only consumer is a single cosmetic string choice. Fine as is, just note it's a very thin piece of state for what it's used for.
- This file has no messiness of its own beyond the missing ABI exposure — it's a small, self-contained reader with clear invariants. Most of the real complexity (and risk) in "boot memory layout" lives in boot/stage2.s's real-mode collection code and the A20/copy-to-1MB sequence, neither of which is in this file.

## Open questions for the rewrite

- Should `mmap_*` gain ABI slots so hand-typed code can inspect the memory map directly, matching the pattern every other subsystem follows?
- Is `MMAP_MAX = 128` still the right ceiling, and should silent truncation (mmap.c:36-37) instead be surfaced (e.g. a count-vs-collected mismatch flag) so a machine with an unusually fragmented map doesn't lose entries invisibly?
- Does the eventual allocator (if/when one exists) want `mmap_largest()`'s single-largest-range policy, or a general iterator over all usable ranges above 1MB? The current API only offers the former as a convenience function alongside raw iteration via `mmap_entry()`.
- Is the E820/E801-fallback distinction (`MMAP_F_E801`) worth keeping as just a cosmetic dump-string flag, or should approximate ranges be handled more conservatively by consumers (e.g. rounding down)?
