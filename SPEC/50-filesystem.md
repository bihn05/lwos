# Filesystem (kernel/fs.c)

**Files:** kernel/fs.c (604 lines), kernel/fs.h (92 lines)
**Status:** built by makefile (`OBJS += kernel/fs.o`, makefile:48); newest subsystem per git log (`82e1d7a Add fs.c`, `3008373 Snapshot before adding fs.c`)

## Purpose

A minimal FAT32 driver against the volume `tools/mkfat.py` lays down at `PART_LBA`: whole-file read and whole-file create/replace, plus a root-directory listing. It is not a general filesystem — no subdirectories, no long file names, no incremental write/seek-and-overwrite, no delete (fs.h:30-36, explicit "deliberately NOT here" list). It exists to move one buffer already assembled in memory (e.g. a `lw_scratch` region the monitor filled by hand) to/from a named file in one call.

Exposed two ways:
- **ABI slots 52-60** (`kernel/abi.h:132-140`) — `fs_init`, `fs_open`, `fs_read`, `fs_size`, `fs_close`, `fs_create`, `fs_replace`, `fs_strerror`, `fs_dump` — so hand-typed asm can `call [0x100000+4*slot]` into any of it, consistent with [[abi-table-contract]].
- **Monitor commands** in kernel/main.c: `sc` (create), `sr` (read), `sw` (replace), `sl` (list) — dispatched at main.c:931-937, implemented by `cmd_file_write`/`cmd_file_read`/`fs_dump` at main.c:577-654. `fs_init(0)` is called once from the monitor's boot path at main.c:1052-1054, hardcoded to `ata_index 0`.

## Public interface

| Function | Slot | Monitor cmd | Signature |
|---|---|---|---|
| `fs_init` | 52 | (auto at boot) | `int fs_init(u8 ata_index)` |
| `fs_open` | 53 | `sr`, `sw` (internal) | `int fs_open(const char *name)` |
| `fs_read` | 54 | `sr` | `int fs_read(int h, void *buf, u32 max_bytes)` |
| `fs_size` | 55 | `sr` (internal) | `int fs_size(int h)` |
| `fs_close` | 56 | `sr` (internal) | `void fs_close(int h)` |
| `fs_create` | 57 | `sc` | `int fs_create(const char *name, const void *buf, u32 len)` |
| `fs_replace` | 58 | `sw` | `int fs_replace(const char *name, const void *buf, u32 len)` |
| `fs_strerror` | 59 | (used by all cmds) | `const char *fs_strerror(int err)` |
| `fs_dump` | 60 | `sl` | `void fs_dump(void)` |

`sw` (replace) goes through `confirm()` before calling `fs_replace` (main.c:597-600) since it's the only one of the four that can destroy existing content — same split the disk-level `w`/`W` commands already use.

## Key structures / state

- `struct fs_geom g` (fs.c:48-58) — one static instance, filled by `fs_init`: `ata_index`, `part_lba`, `fat_start_lba`, `fat_size`, `data_start_lba`, `root_cluster`, `total_clusters`. Nothing is hardcoded from build-time constants; `fs_init` re-derives all of it by reading the MBR partition table then the FAT32 boot sector at runtime (fs.c:105-143), same two reads `boot/stage2.s` already does to find "debug".
- `struct fs_handle handles[FS_MAX_OPEN]` (fs.c:60-66, `FS_MAX_OPEN`=4) — `used`, `first_cluster`, `size`, `cursor`. Global table, not per-caller state — see fs.c:126-132's comment on why (single-threaded monitor, ABI callers "share the monitor's").
- `static u8 fbuf[SEC_SIZE]` (fs.c:73) — **one** shared 512-byte sector buffer for every BPB/FAT/directory/data-sector operation. Explicitly documented as safe only because nothing here is reentrant or interrupt-driven (fs.c:68-72).
- Directory entry field offsets (fs.c:25-34) and FAT32 EOC constants (fs.c:44-46) are hand-decoded byte offsets into raw `u8[512]` buffers, not a packed struct overlay — deliberate, per fs.c:8-12, to dodge alignment/strict-aliasing questions.

**Invariants it assumes about the on-disk layout**, all checked (fail loud, not silently) in `fs_init`:
- `fbuf[0x0D] != 1` → `E_FS_NOFS` (fs.c:125): sectors-per-cluster must be exactly 1. Matches `tools/mkfat.py`'s `SECTORS_PER_CLUSTER = 1` (mkfat.py:30).
- `numfats != 2` → `E_FS_NOFS` (fs.c:128): exactly 2 FAT copies. Matches `mkfat.py`'s `NUM_FATS = 2` (mkfat.py:31).
- Both are load-bearing simplifications: `cluster_to_lba` (fs.c:145-150) is `data_start + (cluster - 2)` with no cluster-size multiplier because sectors-per-cluster is asserted to be 1.

## Dependencies

- **kernel/ata.c only**, via `ata_read`/`ata_write`/`ata_strerror` (`#include "ata.h"`, fs.c:15). No AHCI path — `fs_init(0)` in main.c always opens ATA device index 0; there is no equivalent call wired to `kernel/ahci.c`. Confirm whether that's intentional (AHCI not yet exposed to the monitor) or a gap before rewriting.
- **kernel/console.c** for `con_puts`/`con_hex32`/etc. in `fs_dump` only.
- Called by: kernel/main.c's monitor command dispatch (`sc`/`sr`/`sw`/`sl`), and any hand-typed asm going through ABI slots 52-60.

## Design notes worth keeping

- **Crash-safety ordering, not FAT mirroring** (fs.h:14-28, fs.c:234-271, fs.c:504-557): `fs_create`/`fs_replace` always allocate a fresh cluster chain, write data into it, link the FAT, and only as the *last* step repoint the directory entry's cluster+size fields — one 32-byte write that never crosses a sector boundary, riding on single-sector writes being atomic. A crash before that last write leaves the old file untouched; a crash after leaves the new file valid and at worst leaks the old chain. `set_fat_entry` mirrors both FAT copies (fs.c:172-188) to keep the boot sector's "both FATs mirrored" flag honest, but the comment is explicit that mirroring is *not* the crash-safety mechanism — the ordering is. Worth preserving verbatim in a rewrite; it's a real, load-bearing design decision, not incidental.
- **`short_name()` mirrors `tools/mkfat.py`'s `short_name()`** (fs.c:273-313 vs mkfat.py's own): same split-on-last-dot, uppercase, illegal-char-drop, truncate-to-8+3 rules, best-effort with no collision numbering. Two independent reimplementations of the same rule — see Messiness below.
- **No free-cluster hint is trusted** (fs.c:190-193): FSInfo's next-free field is deliberately left `0xFFFFFFFF` by `mkfat.py` ("let the reader scan"), so `find_free_cluster` always linear-scans from a caller-given start. Fine for this volume size (~64MiB test image); a real filesystem review of this code should flag it as the linear-scan-appropriate-only-at-toy-scale design it is.
- **This does NOT belong in `kernel/binfo.h`** (fs.h:10-12): explicit rationale that `binfo.h` is for real-mode-only or destructive-probe-only data, and `ata_read` works at any time, so FAT geometry is not a boot-harvest candidate. Consistent with [[harvest-block-design]]'s stated test for inclusion.

## Messiness / review notes

This is by a wide margin the *cleanest* file in the kernel/ tree, not the messiest — every non-obvious decision is commented with its rationale, and the error-code design (`E_FS_*` at fs.h:45-55, disjoint from `ata.h`'s `E_*` range, with `fs_strerror` falling back to `ata_strerror` for anything unrecognized, fs.c:86-103) is more disciplined than several older files. That said:

1. **Duplicated FAT32 knowledge with `tools/mkfat.py`, no shared source of truth** (explicitly flagged in the makefile: "neither tool can see the other's source", makefile:33). Concretely duplicated constants/logic:
   - `SECTORS_PER_CLUSTER = 1` (mkfat.py:30) vs the runtime check `fbuf[0x0D] != 1` (fs.c:125) — currently in sync, but nothing enforces it; a future `mkfat.py` change to multi-sector clusters would make every existing image `E_FS_NOFS` under the new driver without an obvious diagnostic pointing at the mismatch.
   - `NUM_FATS = 2` (mkfat.py:31) vs `numfats != 2` (fs.c:128) — same risk.
   - `short_name()` logic exists twice (fs.c:278-313, mkfat.py's own) with matching but independently-typed character-filter lists (fs.c:293-295 and fs.c:305-307 repeat the same 15-character blocklist inline twice within the same function, rather than factoring it once). A single-character-set edit to one blocklist and not the other, or to mkfat.py's Python version, silently diverges 8.3 name derivation between what the host writes and what the kernel expects to find.
   - **Worth deciding in the rewrite:** whether this duplication is acceptable (two tools by design, per the makefile's own stance) or whether a shared machine-readable geometry description (a header both `nasm`/`gcc` and Python could parse, or at minimum a single doc-comment cross-referenced from both — spec/50-filesystem.md and this file already do some of that job manually) is worth building before the format changes again.

2. **No root-directory extension** (fs.h:33-36, fs.c:365 comment): `find_free_dir_slot` returns "root directory is full" (`E_FS_NOFREE`) once the root chain — sized once at image-build time by `mkfat.py`'s `root_clusters` calculation (mkfat.py: computed from `root_entries_needed` at build) — is exhausted. Not a bug, but a hard ceiling on file count baked into the image at `make bin` time; a monitor session that creates enough files can hit it, and the only fix is rebuilding the image with more headroom. Fine for current usage (a dev/debug workflow), worth flagging as a scaling wall if this ever needs to hold more than a handful of files.

3. **Single shared `fbuf[512]`** (fs.c:68-73) is fine under the current single-threaded monitor-loop calling convention, but is a landmine if a future rewrite ever calls `fs_*` from an interrupt handler or makes the ABI table callable reentrantly (e.g. from a timer ISR) — nothing in the code enforces the non-reentrancy assumption, it's comment-only.

4. **`fs_init(0)` hardcodes ATA index 0** at the one call site (main.c:1052) — there's no monitor command to pick a different device, and no AHCI equivalent. If the eventual target has an AHCI-attached boot volume this whole subsystem doesn't reach it yet.

5. Minor: `write_dirent`'s "no RTC read in this kernel yet" (fs.c:407) means every file's create/write/access timestamps are zeroed. Not wrong, just worth listing as a known gap rather than rediscovering it later.

## Open questions for the rewrite

- Keep the single-shared-`fbuf` non-reentrancy assumption, or make buffer ownership explicit (per-handle or per-call) now, before anything depends on the current shape?
- Is the FAT32 format itself staying, or is this the point where a simpler on-disk format (given the project already writes its own image builder) replaces FAT32 entirely, dropping the two-implementations-of-the-same-spec problem outright?
- Root-directory-only, no subdirectories: still acceptable, or does the eventual use case need a directory tree?
- Whole-file replace only, no incremental write: still matches the actual dev workflow (assemble in memory, then commit), or has a caller emerged that wants seek-and-overwrite?
- Should `fs_init` take/probe an AHCI device, or is ATA-only intentional for the life of this project?
- Is `E_FS_*`/`ata_strerror()`-fallback error-code layering (disjoint negative ranges) the pattern to keep for every future subsystem, or worth formalizing (e.g. one shared error-code space with an enum) now that a third subsystem's errors would need a third disjoint range?
