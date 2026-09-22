# Boot: MBR

**Files:** boot/mbr.s (152 lines)
**Status:** assembled standalone by the makefile's `$(MBR)` rule (`nasm -f bin -DIMG_SECTORS=... -DPART_LBA=...`), not part of `$(OBJS)`/the ELF link at all. Written raw to sector 0 of the image by the `$(IMG)` recipe (`dd ... seek=0`).

## Purpose
This is sector 0 of the disk image, loaded to 0x7c00 by the BIOS exactly as every x86 MBR is. Its only job is to get the real-mode stub (boot/stage2.s, assembled separately) off disk and into memory at 0x900, then jump to it. It used to also carry the kernel itself inside its single `int 13h` read, which capped the whole image at 57 sectors; as of the 2026-09-05 rework that job moved to the stub's own read (see spec/11-boot-stage2.md), so this file now only ever moves 8 sectors (`STAGE2_SECTS`).

It also carries the standard PC partition table at 0x1BE, describing the one real FAT32 volume the rest of the system boots from and that `kernel/fs.c` reads/writes at runtime.

## Public interface
- Entry: BIOS jumps here with `dl` = boot drive, `cs:ip` = 0000:7C00.
- Exit: `jmp STAGE2_SEG:STAGE2_OFF` (0000:0900) with `dl` = boot drive, having zeroed `ss/ds/es` and set `sp = bp = 0x7c00`.
- No ABI slots — this file predates the ABI table and never touches 1MB.
- Calling convention: none, it's a straight-line loader with no reusable routines except the local `print`/`disk_err` helpers.

## Key structures / state
- `dap` (line 95): a 16-byte Disk Address Packet for `int 13h AH=42h`, statically initialized (size 0x10, sectors=`STAGE2_SECTS`, buffer 0000:0900, starting LBA 1).
- `boot_drive` (line 123): 1-byte scratch, saved from `dl` at entry, defaulted to 0x80 in case something reads it before entry runs (it can't — dead default).
- Partition table at offset 0x1BE (line 142): one active entry, type 0x0C (FAT32 LBA), `PART_LBA`/`IMG_SECTORS - PART_LBA` supplied via `-D` from the makefile. CHS fields are the standard 0xFE/0xFF/0xFF "not representable" sentinel. Three unused zeroed entries follow.
- Boot signature `0xAA55` at offset 510.

## Dependencies
- `IMG_SECTORS` and `PART_LBA` — build-time `-D` constants, must agree with `tools/mkfat.py`'s own invocation and with `link.ld`'s `PART_LBA` usage in boot/stage2.s. Three independent tools describe one disk layout; nothing enforces agreement except the makefile passing the same values to all three (visible in the makefile's `IMG_SECTORS`/`PART_LBA` block, lines 31-39).
- `STAGE2_SECTS` (line 23, =8) must match `STUB_SECTS` in link.ld — link.ld asserts the stub actually fits in 8 sectors, but nothing asserts this file's constant equals link.ld's; they're just both hand-set to 8.
- Downstream: boot/stage2.s expects to be entered at 0000:0900 in real mode with `dl` = boot drive and nothing else assumed about machine state beyond what the BIOS itself guarantees.

## Design notes worth keeping
- LBA-with-CHS-fallback read (lines 46-69): tries `int 13h` extensions (AH=41h) first, checking the 0x55AA/0xAA55 signature flip and bit 0 of `cl` for packet-style I/O, only falling back to CHS if extensions are absent *or* the AH=42h read itself fails outright. The comment at line 47-51 explains why: CHS needs the BIOS to report ≥9 sectors/track, LBA has no such constraint, but extensions could in principle be advertised and still fail.
- The MBR never reads its own partition table (line 133-134 comment) — it loads the stub by hardcoded LBA 1, so there's no ordering hazard between "read partition 1" and "read the stub," even though both structures live in the same first-64KB region conceptually.
- `dl` is explicitly re-loaded from `[boot_drive]` before the final jump (line 89) because `int 13h` is not documented to preserve it — a subtle correctness fix that's easy to regress if someone "simplifies" this to `jmp` without the reload.

## Messiness / review notes
- Line 33-35 admits an unhandled assumption: "Assuming 0x80 happens to work when that is the only disk, and silently reads the wrong one otherwise." This is a real gap for any multi-disk boot scenario (e.g. booting a USB stick while an internal HDD is also 0x80-eligible on some BIOSes) — there's no comment elsewhere in the codebase resolving it, and boot/stage2.s inherits the same unchecked `dl` on line 126 of that file.
- `print`/`disk_err`/`str_boot`/`str_err` (lines 103-125) are a tiny, self-contained string/print routine duplicated nearly verbatim as `print16` in boot/stage2.s (lines 732-742). Two copies of the same 8-line teletype loop across the two files — a candidate for a single shared include if the real-mode code is ever unified, though nasm has no natural shared-object story across a `.s`/`.s` boundary here (stage2 is a separate `nasm` invocation with its own org).
- The partition-table comment block (lines 127-140) documents three separate historical facts (old fixed-sector kernel load, current file-based load, CHS-not-used) in one place — informative, but it means understanding *this table* requires already knowing the boot/stage2.s FAT-walk design. That's a sign the comment really belongs partly in a higher-level architecture note rather than repeated at each site (this project doesn't have one central doc yet — that's presumably part of what this spec/ directory is for).
- `STAGE2_SECTS` here and `STUB_SECTS` in link.ld encode the identical fact (8 sectors) in two unrelated files with no shared source of truth and no build-time cross-check. link.ld's ASSERT only checks the stub fits within *its own* constant; if someone changes one without the other, the failure mode is a silent short-read of the stub in the CHS path, or nothing at all in the LBA path (LBA reads exactly `STAGE2_SECTS` regardless of the linked stub's real size) — a latent inconsistency, not a caught one.

## Open questions for the rewrite
- Should the drive-selection assumption (line 33-35) be resolved at all, or is "netbook only boots one disk" a permanent, acceptable constraint worth stating explicitly rather than leaving as a comment?
- Is it worth a single shared "real-mode print string" routine/macro between mbr.s and stage2.s, given they're separately assembled flat binaries and the duplication is only ~8 lines?
- Should `STAGE2_SECTS`/`STUB_SECTS`/`PART_LBA`/`IMG_SECTORS` move to one generated header (or a tiny script) that emits consistent `-D` flags for all three tools (nasm×2, mkfat.py), closing the "three tools, one undocumented shared layout" gap the comments repeatedly flag?
- Does the partition table's historical commentary belong here, or in a top-level architecture doc this spec/ series might grow into (e.g. spec/00-overview.md)?
