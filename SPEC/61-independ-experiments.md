# independ/ — standalone hand-loaded test programs

**Files:**
- `gfxtest/gfxtest1.s` (36 lines) [+ gitignored build output `gfxtest1`]
- `lwfat/date.s` (45 lines) [+ gitignored `date`]
- `lwfat/test.s` (13 lines) [+ gitignored `test`]
- `planetest/planetest.s` (205 lines) [+ gitignored `planetest.bin`]
- `planetest/planeshow.s` (251 lines) [+ gitignored `planeshow.bin`]
- `strtx.s` (35 lines) [+ gitignored `strtx`]
- `pineview-findings.html` — a standalone styled report (not a program), documenting the Pineview display investigation these tests came out of

**Status:** NOT part of the make build — confirmed no `independ` path appears in `makefile`'s rules (only an unrelated use of the English word "independent" in a comment on line 75). `.gitignore` treats each compiled output as a build artifact sitting next to its own source, but there is no local Makefile or build script anywhere under `independ/` (`find independ -iname "makefile*"` is empty) — these are assembled by hand, one at a time, most likely `nasm -f bin <file>.s -o <output>` given every file opens with `[bits 32]` / `org 0x200000` (flat binary, no ELF sections, no linker).

## Purpose

Standalone flat binaries meant to be hand-loaded into a *running* kernel via the monitor's own primitives — `l 200000 0 <lba> <sects>` to read them off disk to 0x200000, then `r eip 200000` / `g` to jump in — rather than being linked into `kernel.bin` or reached through `make`. This is the practice-and-probe layer described in project memory (`abi-table-contract`): experiments the user writes and runs directly against the live ABI table, hand-typed, before anything graduates into `kernel/`.

## Inventory

**`gfxtest/gfxtest1.s`** — calls three ABI slots directly by hand-typed address (`call [0x1000B8]`, `[0x1000BC]`, `[0x100018]` — cross-reference the slot enum in `kernel/abi.h`/`spec/21-abi-table.md` for current names), then does a raw MMIO write to `0xFD000000` (a GMADR-style graphics aperture address) and traps with `int3`. Still an active integration smoke-test pattern — it calls *into* the real kernel's live ABI table, so it can only drift by ABI slot renumbering, which the append-only rule in `spec/21-abi-table.md` exists specifically to prevent. **Not drifted**, still exercises a real code path.

**`lwfat/date.s`** — implements `con_out_date`, a from-scratch DOS-date formatter ("起始年是2026年" / "start year is 2026" per its own comment) that calls ABI slots `0x100030` and `0x100014` (console/print primitives) three times. This duplicates knowledge that **should** live in `kernel/fs.c` if DOS-date fields are ever surfaced through a directory-listing monitor command — check `spec/50-filesystem.md` for whether `fs.c` already formats FAT directory-entry dates anywhere; if not, this file is the only place that logic currently exists at all, vendored or otherwise, and is worth promoting rather than discarding.

**`lwfat/test.s`** — 13 lines, a minimal `push`/`call`/`int3` smoke test of the calling convention itself (no FAT, no ABI calls despite living under `lwfat/`). Misleadingly named relative to its sibling `date.s`; it isn't a FAT test at all — it looks like a scratch file for verifying stack/calling-convention mechanics that happened to be dropped in the same directory.

**`planetest/planetest.s`** and **`planetest/planeshow.s`** — the most substantial pair here (205 + 251 lines), both carry detailed Chinese design-postmortem comments at the top documenting a real hardware bug found and fixed: v1 of this test hung the machine because (1) register restore was "write back original values" not "undo individual bits," so `STRIDE`/`ADDR`/`DSPBSURF` were left corrupted, and (2) recovery waited on a keyboard poll of the 8042 OBF bit that could simply never assert, combining with the incomplete restore into an unrecoverable hang. `planetest.s`'s own header states the general principle: *"any design that waits for input before recovering is a deadlock the moment the input path breaks."* `planeshow.s` is the display-only sibling — it deliberately does not attempt to return to text mode, since `kernel/tramp.c`'s `gfx_exit()` (see `spec/24-bios-trampoline.md`) is now the real, validated recovery path back to VGA text via `int 10h AX=0003`. These two files predate and were superseded by the trampoline approach — they're the exploratory work that proved hot-switching planes from protected mode without the BIOS's help is fragile, which is exactly the finding `bios-trampoline` memory cites as its motivation.

**`strtx.s`** — a minimal freestanding `strlen` plus one ABI call (`0x100030`), unrelated to FAT/graphics; looks like a standalone string-handling scratch test, smallest and least contextualized file in the directory (no header comment at all, unlike `planetest`/`planeshow`).

**`pineview-findings.html`** — not a program; a self-contained styled HTML report on the Pineview display investigation. This is the write-up the `planetest`/`planeshow` postmortem comments summarize in miniature. Worth keeping as the actual findings document; it does not belong in a "reformat this like a program" pass.

## Design notes worth keeping

- **The recovery-design principle from `planetest.s`'s header is real, hard-won knowledge, independent of the file it's written in**: never gate hardware recovery on an input event that can itself fail to arrive; recover unconditionally (timer-based or unconditional-on-exit), the same shape `gfx_exit()` in `kernel/tramp.c` already follows. This should end up as a comment near `gfx_exit()` itself, or in `spec/24-bios-trampoline.md`, so it survives independently of whether these two `.s` files stay in the tree.
- **The register-restore bug** (restore must replay every field written, not invert only the bits you remember touching) is a general lesson for *any* future save/restore code that pokes hardware state — worth a one-line rule somewhere more permanent than a test file's header comment.
- **`date.s`'s DOS-date decode** is the only place in the repo that currently turns a raw FAT16 date field into a human-readable year/month/day — if directory listing ever needs this, this file (not a reimplementation) is the starting point.

## Messiness / review notes

- No local build file anywhere in `independ/` — each program's exact assembly invocation exists only in the user's memory/shell history, not committed anywhere. If any of these ever needs rebuilding by someone else, the command has to be reverse-engineered from the `[bits 32]` / `org 0x200000` header convention alone.
- `lwfat/test.s` is misnamed: it lives under a directory named for FAT testing but tests nothing FAT-related. Either it predates `date.s` and the directory's purpose shifted under it, or it was misfiled.
- `strtx.s` sits directly under `independ/` with no subdirectory and no header comment, inconsistent with every other test here (`gfxtest/`, `lwfat/`, `planetest/` all group related files with at least a one-line description).
- `planetest.s`/`planeshow.s` are already-superseded exploratory code (superseded by `gfx_enter`/`gfx_exit` in `kernel/tramp.c`) that are still shaped like "the currently correct way to touch display planes" to anyone skimming the directory without reading the header comments closely — a reader could easily mistake fragile, deliberately-bypassed code for the current approach.

## Open questions for the rewrite

- **Delete outright once the design notes above are captured elsewhere:** `planetest.s`, `planeshow.s` (superseded by the BIOS trampoline; keep only the two lessons in their headers, not the code), `lwfat/test.s` (pure calling-convention scratch, no unique knowledge).
- **Promote, don't discard:** `lwfat/date.s`'s DOS-date formatting logic — fold it into `kernel/fs.c` (or a small console-formatting helper) the day directory listing needs human-readable dates, rather than leaving it as the sole copy of that logic in a directory that isn't built.
- **Keep as smoke tests, but document the build command:** `gfxtest1.s` and `strtx.s` still exercise live ABI slots and are cheap sanity checks after an ABI change — worth a one-line comment (or a tiny `independ/README`) giving the exact `nasm` invocation so they don't silently stop being runnable.
- **Rename or relocate `strtx.s`** to make clear what it's testing and why it isn't grouped like its siblings.
- `pineview-findings.html` should be kept as-is; it's documentation, not code, and isn't part of this triage's "delete or promote" question.
