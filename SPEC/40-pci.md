# PCI Enumeration

**Files:** kernel/pci.c (431 lines), kernel/pci.h (59 lines)
**Status:** built by makefile (`OBJS` includes `kernel/pci.o`)

## Purpose

Talks to PCI configuration space through the legacy mechanism-1 port pair
(0xCF8 address / 0xCFC data) — the only mechanism worth supporting, since
mechanism 2 died with the 486 (pci.h:5-10). On top of raw config-space
read/write it provides: a presence check, a brute-force full-bus scan that
prints every function found, a class/subclass search used by other drivers
to locate a controller without walking the tree themselves, and a per-function
detail dump (header fields, BAR decode with sizes, capability list walk).
There is no bridge-tree walk — every bus 0-255 is swept directly, which costs
8192 config reads but finishes instantly at PIO speed and also finds devices
behind bridges a tree walk would need extra code for (pci.h:6-10).

This file is pure mechanism: it does not own or configure any device, it only
reads/writes config space and reports what is there. `ahci_detect()` in
kernel/ahci.c is the only other driver in the tree that consumes it
(`pci_find_class`, `pci_read16/32`, `pci_write32`).

## Public interface

- `u32 pci_read32/u16 pci_read16/u8 pci_read8(bus, dev, fn, off)` — raw
  config-space reads. Not range-checked: caller must keep `dev < 32`,
  `fn < 8`, `off` 4-byte aligned for the 32-bit variant (pci.h:35-36).
- `void pci_write32(bus, dev, fn, off, val)` — raw config-space write.
- `int pci_present(void)` — is there a mechanism-1 host bridge at all.
- `void pci_scan(void)` — monitor command output: one line per function
  found (bdf, class name, vendor:device, IRQ).
- `int pci_find_class(cls, sub, *bus, *dev, *fn)` — first function matching
  a class/subclass pair, same enumeration order as `pci_scan()`. This is the
  function `ahci_detect()` calls (kernel/ahci.c:275) instead of duplicating
  the scan loop.
- `void pci_detail(bus, dev, fn)` — full header dump, BAR decode with sizes,
  capability list. This is the only function that *writes* config space
  (BAR sizing), and it always restores the original value afterward
  (pci.c:264-266).
- `const char *pci_class_name(cls, sub)` / `pci_vendor_name(ven)` — static
  name tables, never return null for class name, return 0 (not printed) for
  an unrecognised vendor.

No ABI slots are documented in this file directly — check kernel/abi.c for
whether `pci_scan`/`pci_detail`/`pci_find_class` are exposed through the
0x100000 table or only reachable from the monitor's command dispatch in
kernel/main.c.

## Key structures / state

No persistent state at all — every function is either a pure config-space
accessor or a stack-local loop. This is the only one of the four driver
files in this batch with zero static/global variables.

Port/register map (pci.h:12-33): `PCI_CFG_ADDR`/`PCI_CFG_DATA` (0xCF8/0xCFC),
header offsets through 0x3D (`PCI_IRQ_PIN`), `PCI_HDR_MULTIFN` = bit 7 of the
header-type byte.

## Dependencies

- Needs `outl/inl` from io.h and `con_*` output from console.h — both used
  only inside pci.c, not exposed further.
- Depended on by kernel/ahci.c (`ahci_detect()` calls `pci_find_class`,
  `pci_read16`, `pci_read32`, `pci_write32`, and reuses `PCI_COMMAND`/
  `PCI_BAR0` from pci.h) — this is the one real cross-driver contract in the
  set reviewed here.
- kernel/main.c presumably wires `pci_scan`/`pci_detail` into monitor
  commands (`p`, per the string literal at pci.c:200) — not verified in this
  pass, confirm against main.c's command table.

## Design notes worth keeping

- `pci_present()` checks that the address port *held* what was written
  before trusting a vendor-ID read of 0xFFFF, because a board with no host
  bridge floats the port and a naive probe would read back 0xFFFF anyway
  and look like "no device" instead of "no PCI at all" (pci.c:40-48).
- BAR sizing follows the spec's only method (write all-ones, read back,
  restore) and is explicitly justified as safe *only* because "no driver
  owns these devices yet, and the console uses legacy 0xb8000 rather than a
  display BAR" (pci.c:257-259) — this assumption breaks the moment any
  driver claims a BAR-mapped device before `pci_detail` runs against it.
- `bar_size()` has a documented off-by-width bugfix baked into the comment
  itself: an I/O BAR only implements the low 16 bits, so the invert-and-add
  arithmetic must be done in 16 bits or an 8-byte region reports as
  0xFFFF0008 (pci.c:268-278). Worth preserving as a test case if this is
  ever rewritten.
- The capability-list walk is bounded (`guard < 48`) specifically so a
  malformed or circular list cannot hang the monitor (pci.c:356-357).
- `pci_find_class` is a second, textually near-identical copy of the same
  triple-nested scan loop in `pci_scan()` — the comment at pci.c:203-205
  acknowledges this exists "so ahci_detect() can find the AHCI controller
  without duplicating this loop," i.e. the duplication was accepted here
  specifically to avoid a *worse* duplication in the caller.

## Messiness / review notes

- **Duplicated scan loop.** `pci_scan()` (pci.c:147-201) and
  `pci_find_class()` (pci.c:206-239) are the same triple-nested bus/dev/fn
  walk with the same multifunction-detection logic (pci.c:171-176 vs.
  221-226), differing only in what happens on a match. This is a clean
  candidate for a shared iterator/callback if the rewrite wants one; the
  original author already flagged the tradeoff in a comment rather than
  hiding it.
- **Unchecked struct-return style.** Nearly every accessor here takes
  `bus`/`dev`/`fn` as three separate byte args rather than a packed BDF
  value or struct — fine for hand-typed asm call sites (see the ABI-table
  memory note on why raw addresses matter to this project), but it does
  mean every call site repeats three arguments that almost always travel
  together.
- **No error signalling from the raw accessors.** `pci_read32` et al. return
  0xFFFFFFFF-shaped garbage indistinguishably from "read succeeded and the
  register really is that value" — every caller has to know out-of-band
  that 0xFFFF in the vendor ID slot means absent. Idiomatic for PCI code
  generally, but worth naming as a design choice rather than an oversight
  if it's kept.
- No signs of ATA/AHCI-style "two parallel implementations" drift in this
  file — pci.c looks internally consistent and appears to be original code
  for this project rather than ported from elsewhere (contrast with ata.c's
  "ported from external/ata.c" banner).

## Open questions for the rewrite

- Keep the brute-force 256-bus sweep, or add a bridge-tree walk now that a
  correctness need (bridges pci_scan can't see) might exist? The current
  code deliberately chose brute force for simplicity — is that still true
  for the rewrite's goals?
- Fold `pci_scan`/`pci_find_class` into one iterator with a callback/filter,
  now that the duplication is visible in one document instead of split
  across two functions?
- Should config-space accessors take a packed `bdf` (or a `struct pci_dev`)
  instead of three loose bytes, given the project's stated practice of
  hand-typing call addresses/arguments from paper?
- Does `pci_present()`'s save/restore-the-address-port trick belong in every
  BAR-sizing-capable driver, or should it become a shared helper once more
  than one file needs to poke 0xCF8 defensively?
- Where should the "no driver owns display BARs yet" assumption behind BAR
  sizing be re-validated once (if) a graphics driver claims one — should
  `pci_detail`/`bar_size` refuse to size a BAR belonging to a device the
  caller says it now owns?
