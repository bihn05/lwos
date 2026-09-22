# IDT setup, exception naming, and the ISR/ctx bridge to boot/isr.s

**Files:** kernel/idt.c (79 lines), kernel/ctx.h (30 lines), kernel/io.h (57 lines)
**Status:** built by makefile (`OBJS` includes `kernel/idt.o`; `boot/isr.o`
is the asm half this file is inseparable from — see Dependencies)

## Purpose

`idt.c` builds and loads the IDT (48 vectors: 32 CPU exceptions/reserved +
16 IRQ slots) and immediately masks both PICs, so no interrupt can ever fire
through this table — every vector that does trigger is a CPU exception, not
a device IRQ. That is what makes the "debuggee" model in
[[30-main-monitor]] work: `g` (go) and `t` (trace) hand control to
untrusted/experimental code via `dbg_enter`, and whatever faults, the fault
lands back in the monitor as an exception, cleanly, with no possibility of an
unrelated timer or keyboard IRQ interrupting mid-instruction.

`ctx.h` and `boot/isr.s` are the other two-thirds of this mechanism (not
covered file-by-file here since isr.s is assembly, but documented enough to
use): isr.s installs one stub per vector in `isr_table[]`, each of which
saves the full register file into a `struct ctx`, decides (via `in_debuggee`)
whether this trap is "the monitor itself just faulted" (→ `dbg_panic`,
unrecoverable) or "the debuggee under `g`/`t` just faulted" (→ return control
to `dbg_enter`'s caller), and `dbg_enter` is the reverse direction: load a
`struct ctx` into real registers and `iret` into it.

`io.h` is the leaf: raw `in`/`out`/MMIO primitives with no logic of its own,
included by nearly everything.

## Public interface

- `idt_init(void)` — builds all 48 gates from `isr_table[]` (defined in
  isr.s), loads them with `lidt`, masks both PICs (`outb(0x21,0xFF)`,
  `outb(0xA1,0xFF)`).
- `vec_name(u32 v)` — human-readable name for a vector: the 22 named
  x86 exceptions, `"IRQ"` for v>=32 (unreachable in practice since PICs stay
  masked), `"reserved"` otherwise. Used by `report_stop()` in main.c and by
  `dbg_panic` below.
- `dbg_panic(struct ctx *c)` — called from isr.s when a fault happens while
  `in_debuggee` is false (i.e., the monitor's own code faulted, not the code
  it's debugging). Prints full state, CR2 if the vector was #PF (14), and
  halts forever. **Does not return** — this is the "something in the kernel
  itself is broken" terminal state.
- From ctx.h: `dbg_enter(struct ctx *c)` (implemented in isr.s) — load `c`
  into the CPU and `iret`; returns (writing state back into the same `c`)
  after any exception.
- From ctx.h (extern, defined in isr.s): `cur_ctx`, `in_debuggee`,
  `isr_table[]`.
- From io.h: `inb/inw/inl`, `outb/outw/outl`, `io_wait()`, `mmio_r8/w8/r32/w32`
  — all `static inline`, no .c file.

## Key structures / state

- `struct gate` (idt.c:7-13) — the raw 8-byte IDT gate descriptor
  (off_lo/sel/zero/flags/off_hi), packed. `flags = 0x8E` on every gate:
  present, DPL=0, 32-bit interrupt gate (interrupt gates, not trap gates —
  IF is cleared on entry, though this barely matters since IF is already
  effectively unused with the PICs masked).
- `static struct gate idt[NVEC]` (NVEC=48) and the `idtr` limit/base pair —
  both file-local, loaded once and never touched again after `idt_init()`.
- `struct ctx` (ctx.h:7-13) — the shared register-state layout. **The offset
  comment at ctx.h:6 is load-bearing**: "These offsets are hardcoded in
  boot/isr.s — change one, change both." There is no assertion or generated
  header enforcing this the way `binfo.h`'s `_Static_assert` enforces its
  offsets against asm (see [[harvest-block-design]] memory) — a divergence
  here would be a silent miscompile, not a build error.
- `EFL_TF` / `EFL_IF` (ctx.h:15-16) — the two eflags bits the monitor cares
  about (single-step trap flag, interrupt flag).

## Dependencies

`idt_init()` depends on `isr_table[]` from isr.s existing and being exactly
`NVEC` (48) entries; nothing checks that count matches at build time (unlike
`link.ld`'s ASSERTs for the boot layout — see [[boot-memory-layout]] memory).
`dbg_panic` depends on `console.c` for all output. Everything that uses `g`/`t`
in the monitor depends on this file having run (`idt_init()` is the second
call in `kmain()`, right after `con_init()`) and on `boot/isr.s`'s stub
table/`dbg_enter`/`in_debuggee` protocol being intact.

## Design notes worth keeping

- Masking both PICs immediately after loading the IDT (idt.c:55-59) is
  explicitly explained: "BIOS leaves the PIC mapped at 0x08-0x0f, right on
  top of the exception vectors. Mask everything so a timer tick does not get
  reported as a #DF." This is what makes IRQ vectors (32-47) permanently
  unreachable in the current design — `vec_name`'s `"IRQ"` branch is
  currently dead code, kept presumably for if/when IRQs get unmasked.
- The monitor-fault-vs-debuggee-fault split (`in_debuggee`, described in the
  Purpose section) is the actual safety mechanism that lets `g`/`t` run
  arbitrary hand-typed code without permanently wedging the machine: a
  debuggee fault is recoverable (back to the prompt), a monitor fault is not
  (`dbg_panic`, halt). This is the load-bearing design of the whole
  debugger and is currently split across two files (idt.c has the C-visible
  half, isr.s has the mechanism) with no single doc describing the protocol
  end-to-end — this spec section is the closest thing.

## Messiness / review notes

- `struct ctx`'s offset contract with isr.s is asserted nowhere. `binfo.h`
  sets a precedent in this codebase (`_Static_assert` against `BINFO_O_*`
  macros) for exactly this kind of cross-language layout coupling; `ctx.h`
  doesn't follow it. A silent offset drift here (e.g. reordering fields)
  would produce register corruption that only shows up as `g`/`t` scrambling
  the debuggee state, which would be a nasty one to debug precisely because
  it breaks the debugger.
- `idt.c` has no build-time check that `NVEC` (48) matches the number of
  entries isr.s actually populates in `isr_table[]`; a mismatch would either
  read past the array or leave gates zeroed silently.
- `vec_name`'s `"IRQ"` branch (idt.c:35-36) is unreachable given the PICs
  are masked at idt_init time and nothing in this kernel ever unmasks them
  — dead code today, though clearly deliberate (kept as a placeholder for
  future IRQ support).
- `dbg_panic` special-cases only vector 14 (#PF) to print CR2 (idt.c:73-77);
  #GP (13), #TS (10), #NP (11), #SS (12) — the other vectors `main.c`'s
  `report_stop()` already knows carry an error code — don't get any extra
  detail here even though they're just as common a source of a monitor-self-
  fault during development. Minor inconsistency between the two fault
  reporters (`dbg_panic` here vs. `report_stop` in main.c) rather than one
  shared "describe this fault" routine.
- `io.h` is a leaf header with no corresponding `.c` — fine as-is (all
  `static inline`), but it's worth noting it's the one file in `kernel/`
  that mixes two genuinely different concerns under one name: port I/O
  (`inb`/`outb`/...) and raw MMIO (`mmio_r8`/`mmio_w8`/...). They're
  unrelated hardware mechanisms (x86 port space vs. memory-mapped access)
  that happen to both be "a byte in, a byte out."

## Open questions for the rewrite

- Add a `_Static_assert`-style (or generated) check that `struct ctx`'s
  layout matches what isr.s assumes, following the precedent already set by
  `binfo.h`? Given how bad a silent drift would be here, this seems like
  the highest-value single addition to this unit.
- Should `NVEC` and isr.s's stub count be tied together at build time
  (e.g. a linker symbol counting isr.s's own table), the way `link.ld`
  already ties several other boot-time constants to computed values instead
  of literals?
- Is unifying `dbg_panic` (idt.c) and `report_stop` (main.c) into one
  "describe a fault" routine worth doing, given they already diverge
  (CR2-on-#PF only in one of them)?
- Keep `io.h`'s port-I/O and MMIO primitives in one file, or split into
  `port_io.h`/`mmio.h` now that the codebase has enough separate concerns
  to make that worth the extra include?
- Any future plan to unmask IRQs (timer, real keyboard IRQ instead of
  polling) — and if so, does the debuggee/monitor fault-isolation model
  described above still hold once real interrupts can land during `g`/`t`?
