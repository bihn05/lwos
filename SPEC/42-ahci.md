# AHCI (Native SATA Driver)

**Files:** kernel/ahci.c (343 lines), kernel/ahci.h (42 lines)
**Status:** built by makefile (`OBJS` includes `kernel/ahci.o`)

## Purpose

Native AHCI (SATA) support: finds an AHCI controller over PCI, takes over
whatever port configuration firmware already established (no COMRESET, no
BIOS/OS handoff), and does polling DMA transfers through a single command
slot per port. This exists because a machine with an AHCI controller has
*nothing* behind the legacy 0x1F0/0x170 ports at all — not "the wrong mode,"
genuinely absent — so `ata_detect()` in kernel/ata.c tries this backend
first and only falls back to legacy PIO scanning if no AHCI controller
answers (ahci.c:1-3, cross-referenced at ata.c:220-224).

## How ata.c and ahci.c coexist

They are both live, and the relationship is a clean two-way split rather
than duplication or dead code:

- **ata.c → ahci.c:** `ata_detect()` calls `ahci_detect()` first (ata.c:224);
  `xfer()` (the shared internal transfer function backing both
  `ata_read`/`ata_write`) dispatches straight to `ahci_rw()` when
  `d->backend == ATA_BACKEND_AHCI`, before any legacy register setup runs
  (ata.c:361-362).
- **ahci.c → ata.c:** `ahci_identify()` calls `ata_identify_parse()`
  (ahci.c:262, defined in ata.c:101-119) so the IDENTIFY-word field
  extraction exists exactly once regardless of transport. `ahci_issue()`
  also reuses `ATA_SR_ERR`/`ATA_SR_DF` from ata.h rather than redefining
  them, because AHCI's `PxTFD` register mirrors the classic ATA status byte
  bit-for-bit (ahci.c:236-238).
- **Shared error space:** both funnel every failure through the same
  `E_ARG`..`E_ATAPI` codes and the one `ata_strerror()` defined in ata.c
  (ata.h:60-61).
- ahci.h's own header comment states the boundary explicitly: "this file
  owns all AHCI-specific state and every register access; ata.c never
  touches an AHCI register directly, it only sees the two functions below
  through struct ata_dev's existing shape" (ahci.h:10-12). That boundary
  holds under inspection — no AHCI register macro or MMIO call appears
  outside ahci.c.

There is no dead code or half-migrated interface between the two files in
this pass — the split reads as a deliberate, currently-consistent design,
not drift. (Contrast with external/ata.c, an entirely separate abandoned
file — see spec/41-ata.md's note and spec/17-external-scratch.md.)

## Public interface

- `u8 ahci_detect(struct ata_dev *devs, u8 max_devices)` — find the
  AHCI controller (PCI class 01/06 via `pci_find_class`), enable it,
  program up to `AHCI_MAX_PORTS` (4, same as `ATA_MAX_DEVICES`) ports found
  present, fill `devs[]` in the same `struct ata_dev` shape the legacy scan
  uses (`backend = ATA_BACKEND_AHCI`, `channel` = AHCI port index). Returns
  the count filled, 0 if no controller/no device (ahci.h:27-33).
- `int ahci_rw(const struct ata_dev *d, u64 lba, u32 sectors, void *buf, int
  write)` — same 0-on-success/negative-`E_*` contract as ata.c's internal
  `xfer()`. Always LBA48 (SATA drives are always LBA48-capable, so there is
  no LBA28 branch) (ahci.h:35-39).

Both are called only from kernel/ata.c in the reviewed set; nothing else in
the tree references them directly.

## Key structures / state

- HBA/port register offset macros for AHCI 1.3.1 (ahci.c:11-49) — generic
  registers (`CAP`,`GHC`,`IS`,`PI`,`VS`) and per-port registers computed via
  `AHCI_PORT_BASE(port) = 0x100 + 0x80*port`.
- On-the-wire structures: `struct ahci_cmd_header` (32B, command-list
  entry), `struct ahci_prdt_entry` (16B, one DMA region descriptor),
  `struct ahci_cmd_table` (64B CFIS + 16B unused ACMD + 48B reserved + one
  PRDT entry, `aligned(128)` so sizeof rounds to 256 and every array element
  stays 128-aligned automatically — ahci.c:69-80).
- Per-port static arrays, capped at `AHCI_MAX_PORTS`: `cmd_list[port][32]`
  (1024-aligned), `fis_recv[port][256]` (256-aligned), `cmd_tbl[port]`
  (ahci.c:86-88). Indexed by *physical AHCI port number*, not by the
  `devs[]` array index — so a device on port 2 with ports 0/1 empty still
  uses `cmd_list[2]` etc., letting `ahci_rw()` go straight from
  `d->channel` to its buffers without an indirection table (ahci.c:82-85).
- `static u16 identify_buf[256]` — shared IDENTIFY scratch, safe because
  `ahci_detect()` probes ports sequentially, never concurrently (ahci.c:90-92).
- `static u32 abar_addr` — 0 until a controller is found; doubles as the
  "is AHCI live" flag checked by `ahci_rw()` (ahci.c:94, 318-319).
- No paging anywhere in this kernel (documented at ahci.h:14-17 and
  ahci.c:6-7), so ABAR and every static buffer are touched as plain
  physical addresses via `mmio_r32`/`mmio_w32` — the same idiom console.c
  uses for the fixed 0xB8000 VGA MMIO window.

## Dependencies

- Includes pci.h and calls `pci_find_class`, `pci_read16/32`, `pci_write32`
  (ahci.c:9, 275-287) — see spec/40-pci.md.
- Includes ata.h and calls `ata_identify_parse` (ahci.c:262), reuses
  `ATA_MAX_DEVICES` as `AHCI_MAX_PORTS`'s value and `ATA_SR_ERR`/`ATA_SR_DF`
  (ahci.c:50, 236-238).
- Called exclusively by kernel/ata.c (`ata_detect()`, `xfer()`).

## Design notes worth keeping

- **AE not HR.** `ahci_detect()` sets only the AE (AHCI Enable) bit in GHC,
  never HR (HBA Reset): "take the controller over as firmware left it
  rather than resetting it — simpler, and avoids BIOS/OS handoff entirely"
  (ahci.c:291-293). This is the same philosophy as skipping BOHC
  (ahci.h:19-21) — the driver is deliberately minimal, not accidentally
  incomplete.
- **PCI COMMAND register write avoids a read-modify-write hazard.** Setting
  Memory Space Enable + Bus Master Enable is done via a 16-bit read + 32-bit
  write with the high half forced to 0, specifically because a naive 32-bit
  RMW would copy the current STATUS half back and silently clear whatever
  write-1-to-clear bits happened to be set there (ahci.c:278-284).
- **Command-engine stop/start ordering is spec-mandated and documented
  in-line:** `port_stop()` clears ST then waits for CR to drop, then clears
  FRE and waits for FR to drop, "required before PxCLB/PxFB may be
  reprogrammed — the HBA is free to be mid-DMA against whatever those
  pointers held before this driver took over" (ahci.c:101-104). `port_start()`
  sets FRE *before* ST — "the spec requires the FIS engine running before
  the command engine starts, not the other way round" (ahci.c:143-144).
- **`port_present()` has no COMRESET.** It retries reading `PxSSTS` for
  DET==3/IPM==1 because firmware may not have finished link training the
  instant this driver takes over, but deliberately never issues a manual
  link reset — "this driver never resets the link, only takes over one
  already negotiated by firmware" (ahci.c:151-157). A device that never
  reports DET==3 in the loop is treated as absent, not kicked.
- **One PRDT entry per command, by design, not by oversight.** ahci.c:69-72
  explains large transfers are chunked into multiple single-region commands
  in `ahci_rw()` rather than building a multi-entry PRDT.
- **8192-sector (4MB) chunking in `ahci_rw()`** matches the PRDT's 22-bit
  byte-count field ceiling, documented both at the struct definition
  (ahci.c:66) and at the call site (ahci.c:198-200, 330).
- **Results are written through `PxTFD`'s low byte**, explicitly reused as
  the classic ATA status byte rather than redefining error bits
  (ahci.c:236-238) — mirrors the same "share, don't duplicate" instinct
  seen in `ata_identify_parse`.

## Messiness / review notes

- **ATAPI is detected but not usable.** `ahci_identify()` correctly
  distinguishes `AHCI_SIG_ATAPI` and sets `d->type` accordingly (ahci.c:245-
  251), so `ad`/monitor listings show ATAPI devices — but `ahci_rw()` never
  checks `d->type`, and read/write for an ATAPI device would attempt
  `AHCI_CMD_READ_DMA_EXT`/`WRITE_DMA_EXT`, which are not valid ATAPI
  commands. The rejection actually happens one layer up, in ata.c's shared
  `xfer()` (`if (d->type == ATA_DEV_PATAPI) return E_ATAPI;`, ata.c:358-359)
  — so this is correct in practice, but it means ahci.c's own file-level
  comment ("read/write stays E_ATAPI, same as the legacy backend," ahci.h:
  23-24) describes behavior that is actually enforced by the *caller*, not
  by this file. Worth flagging as a spot where the ownership boundary
  stated in the header comment doesn't quite match where the check lives.
- **`ahci_issue()`'s completion poll treats a set TFES (TFD error) bit as a
  reason to break the wait loop, but then still reads `PxTFD` and returns
  based on its contents** (ahci.c:223-241) — correct, but the two exit paths
  (CI-cleared vs. TFES-set) are not distinguished in the returned error
  code; both funnel through the same `tfd`-bit check. Not a bug, but a place
  where slightly more detail (timeout vs. device-reported error vs.
  transport error) could help future debugging.
- **`identify_buf` being a single shared buffer is safe only as long as
  `ahci_detect()` stays sequential** — the comment justifies it (ahci.c:90-
  92) but nothing enforces the invariant structurally; a future change that
  parallelizes port probing would silently corrupt it.
- No parallel/duplicate-implementation drift found inside this file itself
  — the AHCI/legacy split is the one place in this subsystem where two
  "implementations" of the same concept (disk I/O) coexist *on purpose* and
  *by clean interface*, which is worth calling out as a positive example
  next to external/ata.c's drift.

## Open questions for the rewrite

- Should the ATAPI rejection move into `ahci_rw()`/`ahci_issue()` directly,
  so ahci.c's contract is self-contained rather than depending on ata.c's
  `xfer()` catching it first? (Today it's correct but the enforcement point
  doesn't match the header comment's claim.)
- Is BIOS/OS handoff (BOHC) or a COMRESET-capable link-reset path something
  the rewrite needs, or does "only ever take over what firmware already
  negotiated" remain an acceptable permanent constraint (it holds on QEMU
  and typical desktop firmware per ahci.h:20-21, but may not hold on all
  target hardware)?
- Multi-entry PRDT support (removing the 4MB-chunk-per-command limit) —
  worth it, or is one-PRDT-entry-per-command simplicity worth keeping given
  this driver's polling, no-NCQ, single-slot design already trades
  throughput for simplicity everywhere else?
- Should `AHCI_MAX_PORTS` stop silently reusing `ATA_MAX_DEVICES`'s value
  and become an independently-justified constant, now that the coupling is
  visible in one document?
