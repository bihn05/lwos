# ATA (Legacy PIO Driver)

**Files:** kernel/ata.c (430 lines), kernel/ata.h (121 lines)
**Status:** built by makefile (`OBJS` includes `kernel/ata.o`) — this is the
ATA implementation that actually ships in the kernel image.

> **This chapter is about kernel/ata.c only.** There is a second, unrelated
> file at `external/ata.c` / `external/ata.h` in this repository. It uses a
> different naming convention (`ata_device_t`, `ata_init()` with no args,
> global `ata_devices[]`/`ata_device_count`), includes `<driver/ata.h>` which
> does not match this repo's actual layout, and **is not referenced anywhere
> in the makefile's `OBJS` list** — it does not compile into anything. Per
> kernel/ata.c's own header comment, kernel/ata.c was *ported from*
> external/ata.c but is now a distinct, actively maintained file (port order
> of `outb()` was flipped, a missing `FLUSH CACHE` after writes was added —
> see below). Treat external/ata.c as leftover source-of-a-port material, not
> a live alternative implementation; its own chapter (written separately)
> covers what it contains and why it's still in the tree.

## Purpose

PIO-mode ATA (IDE) disk access over the legacy 0x1F0/0x170 port pairs:
device probing (`ata_detect`), IDENTIFY parsing shared with the AHCI backend,
and sector read/write in both LBA28 and LBA48 addressing, polling only — no
interrupts, no DMA (ata.h:5). This is the fallback backend: `ata_detect()`
tries AHCI first (see spec/42-ahci.md) and only falls back to walking the
four legacy primary/secondary × master/slave positions if no AHCI controller
answered (ata.c:220-224).

## Public interface

- `void ata_init(void)` — zero the device table, mask IRQ14/15 on both
  channels since this driver polls (ata.c:180-189).
- `void ata_detect(void)` — probe and print. Tries `ahci_detect()` first;
  only runs the legacy 4-position PIO scan if that returns 0 devices
  (ata.c:213-301).
- `u8 ata_count(void)` / `const struct ata_dev *ata_get(u8 index)` —
  accessors into the last `ata_detect()` result.
- `int ata_read(index, lba, sectors, buf)` / `int ata_write(index, lba,
  sectors, const buf)` — sector transfer, 0 on success, negative `E_*` on
  error (ata.h:101-111). Picks LBA28 vs LBA48 automatically by device
  capability and requested LBA/count size.
- `const char *ata_strerror(int err)` — human-readable error text, shared
  with kernel/ahci.c per ata.h:60-61 and ata.c:12-27.
- `void ata_identify_parse(struct ata_dev *d, const u16 id[256])` —
  transport-independent IDENTIFY-word parser. Exported specifically so
  kernel/ahci.c can reuse it (ata.h:115-119, ata.c:101-119) rather than
  reimplement the byte-swap/trim logic for the AHCI backend.

No direct ABI-table involvement is visible in this file; whatever exposes
disk I/O to hand-typed code (fs.c, or an ABI slot) sits above this layer —
check kernel/abi.c / kernel/fs.c for how `ata_read`/`ata_write` get reached
from outside the kernel.

## Key structures / state

- `static struct ata_dev devs[ATA_MAX_DEVICES]` (ATA_MAX_DEVICES = 4) and
  `static u8 n_devs` — the only state, filled once per `ata_detect()` call
  and otherwise read-only (ata.c:9-10).
- `struct ata_dev` (ata.h:75-91): `present`, `type` (`ATA_DEV_PATA` /
  `ATA_DEV_PATAPI`), `channel`/`drive` (legacy: primary/secondary,
  master/slave; **reinterpreted** for AHCI: `channel` becomes the AHCI port
  index and `drive` is unused/always 0 — see ata.h:78-79), `lba48`,
  `signature`, `sectors`, `model[41]`/`serial[21]`/`firmware[9]` (IDENTIFY
  strings), and `backend` (`ATA_BACKEND_LEGACY` or `ATA_BACKEND_AHCI`,
  ata.h:72-73, 90). A comment at ata.h:86-89 notes `backend` was
  deliberately appended after `firmware` rather than inserted earlier, to
  land in existing struct padding and avoid shifting any `ABI_DEV_O_*`
  offset used from asm — direct evidence the struct layout is itself part
  of the hand-typed-address contract (see the ABI-table-contract project
  memory).
- Port/register/command/status-bit macros (ata.h:11-51) for both legacy
  0x1F0/0x170 bases and the LBA48 two-deep-FIFO register reuse
  (`ATA_REG_SECCOUNT1`/`LBA3..5` alias the low registers, ata.h:27-32).
- Error codes `E_ARG` .. `E_ATAPI` (ata.h:62-70) are a small negative-int
  enum shared with ahci.c.

## Dependencies

- Includes ahci.h and console.h; calls `ahci_detect()` and `ahci_rw()` from
  kernel/ahci.c (ata.c:224, 361-362) — this is a two-way coupling: ata.c
  calls into ahci.c for detection/transfer, and ahci.c calls back into
  ata.c's `ata_identify_parse()` (see spec/42-ahci.md). Both funnel errors
  through the one `ata_strerror()` defined here.
- Nothing in the reviewed set calls `ata_read`/`ata_write`/`ata_detect`
  directly except presumably kernel/main.c (monitor `ad` command, per the
  `chan_name`/`drv_name` print strings) and kernel/fs.c (disk I/O for the
  FAT32 layer) — not verified in this pass.

## Design notes worth keeping

- **The `outb()` argument-order trap.** ata.c:1-4 calls this out explicitly:
  the code this was ported from used `outb(value, port)`; this project uses
  `outb(port, value)`. Anyone diffing kernel/ata.c against external/ata.c
  line-by-line needs to know every `outb`/`outw` call has its arguments
  swapped, not just renamed.
- **`FLUSH CACHE` was added, not inherited.** ata.c:408-416 sends
  `ATA_CMD_FLUSH_CACHE`/`_E` after every write and treats a failed flush as
  `E_FLUSH`; the comment says "the original driver defined the command and
  never sent it" (ata.c:409-411) — i.e. external/ata.c has this command
  *defined* but dead. This is a real correctness fix made during the port,
  worth preserving as a fact even if the code around it is rewritten.
- **LBA48 register-write order is load-bearing.** ata.c:332-334: high bytes
  must be written first because each register is a two-deep FIFO and the
  drive takes the *first* write as the high half — a subtle hardware detail
  easy to silently break by reordering during a refactor.
- **`ata_delay400`** reads the *alternate* status register (control port)
  purely for its timing side effect, because reading it — unlike the real
  status register — has no effect on pending interrupts/state (ata.c:32-34).
- **Backend dispatch is a single `if` inside `xfer()`** (ata.c:361-362):
  `d->backend == ATA_BACKEND_AHCI` short-circuits straight to `ahci_rw()`
  before any of the legacy LBA28/LBA48 setup code runs. All the argument
  validation above that line (`E_NODEV`, `E_ARG`, `E_ATAPI` checks at
  ata.c:354-359) is shared by both backends; only the register-poking core
  differs.
- **AHCI alignment requirement leaks through this shared entry point.**
  ata.h:106-109 documents that AHCI-backed transfers additionally require
  `buf` to be at least word-aligned (the PRDT needs it) while legacy PIO has
  no such requirement — "a real, if obscure, behavioural difference between
  backends" that callers of `ata_read`/`ata_write` must know about even
  though the function signature gives no hint which backend a given `index`
  uses.

## Messiness / review notes

- **Two IDENTIFY paths, no shared struct with ahci.c beyond the parse
  function.** ahci.c has its own `ahci_identify()` (ahci.c:245-264) that
  duplicates the "clear type, call the shared parser" pattern rather than
  ata.c owning identify dispatch entirely — reasonable given the transport
  difference, but means the two backends' identify flows have to be read
  side-by-side to see they agree.
- **`ata_wait()`'s 0xFF-means-floating-bus check exists only in the legacy
  path** (ata.c:51-52) — AHCI's `ahci_issue()` has no equivalent "is
  anything there" bail-out beyond the timeout, relying instead on
  `port_present()` at detect time. Not a bug, but an asymmetry between the
  two backends' failure modes worth flagging if unifying error semantics is
  a rewrite goal.
- **Legacy detect's "scratch test" (write 0x55/0xAA, read back) exists only
  in `ata_detect()`'s inline loop** (ata.c:262-269), not as a named helper —
  a minor case of logic embedded in a large function rather than factored
  out, in an otherwise fairly clean file.
- **`print_dev_tail`/`chan_name`/`drv_name` mix presentation (monitor
  output formatting) into the same file as the transfer logic** (ata.c:200-
  211, 215-216) — not unusual for this codebase's style (console.c is
  pulled into most drivers), but worth deciding during the rewrite whether
  probing/printing should split into separate translation units per layer.
- No evidence in kernel/ata.c itself of "changed requirements" drift — the
  actual drift in this subsystem is the *existence* of external/ata.c as an
  abandoned parallel implementation, not anything inside this file. See
  spec/17-external-scratch.md (if written) for that side.

## Open questions for the rewrite

- Should `struct ata_dev`'s dual meaning for `channel`/`drive` (legacy
  channel+drive vs. AHCI port index, unused drive) become two distinct
  fields, or is the union-by-convention worth keeping for ABI-offset
  stability reasons already documented in the struct comment?
- Does the AHCI alignment requirement belong surfaced in `ata_read`/
  `ata_write`'s contract explicitly (e.g. always word-align, unconditionally,
  regardless of backend) rather than being a backend-dependent surprise?
- Is polling-only (no IRQ14/15, no DMA on the legacy path) a permanent
  design constraint, or a placeholder to revisit once interrupt handling
  matures elsewhere in the kernel?
- Should `ata_strerror`'s shared error-code space with ahci.c move into a
  shared errno-style header now, rather than living in ata.h and being
  documented as "shared" by comment convention only?
