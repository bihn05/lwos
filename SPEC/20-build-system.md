# Build System (makefile + link.ld)

**Files:** makefile (132 lines), link.ld (106 lines)
**Status:** the makefile drives every other build; link.ld is the `-T` script in its `LDFLAGS` (makefile:23)

## Purpose

Two problems solved together: (1) compile/assemble/link a real-mode boot stub and a 32-bit kernel that must end up as two *separate* flat binaries even though they are one link unit, and (2) assemble a bootable FAT32 disk image from those binaries plus a filesystem root directory.

The single-ELF-two-binaries trick exists because both halves share compile-time knowledge (the ABI table, symbol addresses) that's easiest to keep consistent through one linker pass, but they load to different places at different times: the stub is read by the MBR to a fixed LBA in real mode (makefile:54-55, boot/mbr.s), the kernel is found later as a file named `debug` inside the FAT32 partition by the stub's own filesystem walk (makefile:87-93, link.ld:1-13). `objcopy --only-section` (makefile:81-85) cuts the one ELF into `.stage2.entry` and `.kernel` pieces post-link.

## Public interface

Make targets (makefile:52,95,105-113,120,128):
- `all` / `bin` → builds `$(IMG)` (lwcnc.img)
- `resetimg` → force-rebuilds the image from scratch, working around make's mtime blindness to in-place image writes (makefile:97-104)
- `run` → boots the image in Bochs
- `pgm` → writes the image to `$(DEVICE)` (`/dev/sda` by default, makefile:50,112-113) via `pgm.sh`
- `map` → prints the live ABI table (slot, address, current target, name) read straight out of `kernel.bin`, no boot required (makefile:115-126)
- `clean` → removes objects and generated binaries

Build-time contract constants shared with other tools (makefile:38-39):
- `IMG_SECTORS = 131040` — total image size, must agree with boot/mbr.s's partition table (`-D` flag, makefile:55) and bochsrc's CHS geometry
- `PART_LBA = 2048` — FAT32 partition start, passed to both boot/mbr.s and boot/stage2.s (`-DPART_LBA`, makefile:62) and to `tools/mkfat.py` (makefile:92)

link.ld exports (link.ld:43,51,57,62,64,69,74,78-79,82): `__stub_end`, `__kernel_lma`, `__kernel_vma`, `__abi_start`, `__abi_end`, `__kernel_vma_end`, `__bss_start`, `__stack_top`, `__bss_end`, `__kernel_size`. `KERNEL_LOAD_PHYS` (link.ld:35) and `STUB_SECTS` (link.ld:30) are the two numbers boot/stage2.s and boot/mbr.s must agree with.

## Key structures / state

- `OBJS` (makefile:46-48) — the explicit link order. `kernel/abi.o` **must** be first so `.abi` lands at 0x100000; the comment at makefile:44-45 says the ordering exists only so the link.ld ASSERT never has to fire, i.e. it's a belt-and-suspenders duplicate of a check link.ld already makes authoritative.
- link.ld section layout (link.ld:37-105): `.stage2.entry` at 0x900, `.kernel` at 0x100000 (with `.abi` forced first inside it), `.bss` (NOLOAD) following at 1MB+kernel size, sized to hold a 16KB stack (`STACK_SIZE`, link.ld:26).
- Four `ASSERT`s (link.ld:85-102) are the actual guardrails: ABI table at exactly 0x100000, ABI table non-empty, stub fits its sector budget and stays under 0x7c00, kernel fits below the 0xA0000 VGA hole in real mode.

## Dependencies

- boot/mbr.s: reads `STAGE2_SECTS` sectors from a fixed LBA into `.stage2.entry`'s load address (0x900); its own `-D` flags for `IMG_SECTORS`/`PART_LBA` must match this makefile's.
- boot/stage2.s: consumes `PART_LBA` (walks the FAT32 partition itself for the kernel) and the link.ld symbols for where to copy the kernel and where BSS/stack live.
- kernel/abi.c: `.abi` section placement is what `map`'s address arithmetic (makefile:120-126) and the ASSERTs both depend on.
- tools/mkfat.py: consumes `IMG_SECTORS`, `PART_LBA`, `$(FSROOT)`, and the kernel binary under the name `debug` (makefile:92-93) — this script is *not* in the file list reviewed here and is a hard dependency worth reading before touching image layout.
- kernel/%.o pattern rule (makefile:67-70) lists nearly every kernel header as a prerequisite by hand — this is a manual substitute for `-MMD`/dependency-file generation.

## Design notes worth keeping

- The `-Os` vs `-O2` history in the CFLAGS comment (makefile:9-19) is a good example of a constraint that used to be load-bearing (57-sector MBR ceiling) and no longer is (kernel is now loaded by the stub itself, ceiling moved to ~576KB at the VGA hole) — the flag choice today is admittedly just preference, not a constraint. Worth deciding explicitly in the rewrite rather than carrying it forward as unexamined tradition.
- `resetimg` (makefile:97-104) documents a real make footgun: writing into the image in-place (mounting it, or the kernel's own fs write path over QEMU) bumps its mtime past every prerequisite, so plain `make bin` thinks it's already up to date and leaves stray test content in. This is exactly the kind of trap worth keeping documented even if the mechanism changes.
- `map`'s slot-count arithmetic (makefile:122-124) derives `n` from `__abi_start`/`__abi_end` via `nm`, rather than hardcoding `LW_SLOT_COUNT` — so it stays correct even if the ABI table grows, at the cost of being fairly opaque shell.
- link.ld's per-ASSERT comments (link.ld:84,90-92,98-100) each explain *which* invariant would silently break and how — this is the clearest "spec as comments" writing in the whole boot chain and is worth preserving in whatever replaces link.ld.

## Messiness / review notes

- `__kernel_lma` (link.ld:45-51) is explicitly stale: the comment says it's "not a load address any more" and is kept only because `kernel/main.c`'s `fill` safety check still uses it, with the formula unchanged from when it *did* double as the kernel's load address. This is exactly the kind of vestigial cross-file coupling the user is trying to unwind — grep `kernel/main.c` for `fill` before deleting.
- Two independent statements of "how many sectors is the stub": `STAGE2_SECTS` in boot/mbr.s (referenced but not defined here) and `STUB_SECTS` in link.ld (link.ld:30). The comment at link.ld:27-29 says they "must match" — this is a manual invariant with no build-time check tying the two together (unlike the ABI/VGA-hole invariants, which *do* have ASSERTs). A drift here fails silently or produces a truncated stub.
- The makefile pattern rule at makefile:67-70 hand-lists kernel headers as prerequisites; if a `.c` file starts including a header not in that list, make won't rebuild it on that header's changes. This is brittle by construction and will only get worse as files are added — a candidate for `-MMD -MP` if the rewrite still uses `gcc`+`make`.
- `DEVICE = /dev/sda` (makefile:50) hardcoded as the `pgm` target's default — flashing to a wrong block device is a classic footgun; worth at least an interactive confirmation or an existence/size check in a rewrite, though `pgm.sh` (not reviewed here) may already guard this.
- No `.PHONY` entry is missing, but `bin` is both a real prerequisite chain and a phony convenience name for `$(IMG)` — fine as is, just note that `all` and `bin` are synonyms with no distinction, which is one more name than needed.

## Open questions for the rewrite

- Does the rewrite keep the "one ELF, two objcopy'd binaries" trick, or move to genuinely separate link units for the stub and kernel now that there's headroom (576KB) and no more reason to share a single link pass?
- Is `make` itself being kept, or replaced? If kept, is proper header dependency tracking (`-MMD -MP`) worth adding now rather than carried forward as a hand-maintained list?
- Should `IMG_SECTORS`/`PART_LBA` move to one place both nasm and mkfat.py read (e.g. a generated `.inc`/`.py` file) instead of being typed twice across makefile flags?
- Is the `-Os`/`-O2` choice worth revisiting now that it's confirmed to be preference rather than a hard constraint?
- Does `__kernel_lma` and its consumer in `kernel/main.c`'s `fill` check still deserve to exist, or was its job fully superseded by the newer ASSERTs?
- Is `STAGE2_SECTS`/`STUB_SECTS` duplication worth closing with a shared constant file, given it presently has no build-time cross-check?
