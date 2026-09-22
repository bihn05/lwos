# Console: VGA text + COM1 serial

**Files:** kernel/console.c (238 lines), kernel/console.h (30 lines)
**Status:** built by makefile (`OBJS` includes `kernel/console.o`)

## Purpose

The only output device this kernel has is an 80x25 VGA text-mode framebuffer
at 0xB8000, mirrored byte-for-byte to a polled COM1 serial port. The only
input devices are the PS/2 keyboard (via `kbd.h`, decoded to a stream of
ASCII bytes and `K_*` named-key codes — see [[kbd decoder]] memory) and that
same COM1 port. `console.c` is the single point where both output paths and
both input paths are fused into the four calls (`con_putc`/`con_puts`,
`con_getc`/`con_getkey`) that every other kernel unit — chiefly the monitor in
[[30-main-monitor]] — uses instead of touching hardware directly.

Because there is no interrupt-driven I/O anywhere in this kernel (see
[[32-idt]] — the PIC is masked entirely), everything here is polling: writes
to COM1 spin-wait on the line-status register, reads check both the keyboard
scancode decoder and the serial RX-ready bit on every call.

## Public interface

- `con_init(void)` — clears the VGA screen, programs COM1 to 115200 8N1 with
  FIFO on and interrupts off, runs a loopback self-test to detect whether a
  real UART is present, then drains any stale byte left in the 8042 output
  buffer by the BIOS.
- `con_reset_cursor(void)` — cursor-only reset, no hardware touched but the
  CRTC index registers. Exists specifically so callers who need to recover
  the screen after something *external* clears it (the BIOS, on a video mode
  set — see [[bios trampoline]] memory) don't have to call `con_init()` and
  lose queued serial input.
- `con_putc(char)`, `con_puts(const char *)`, `con_puts_pad(const char *, int)`
  — output. Handles `\n`, `\r`, `\b`; `con_puts_pad` right-pads with spaces
  for table alignment without needing `strlen` (the build uses
  `-fno-builtin`, so a hand-rolled loop avoids an unexpected libc call).
- `con_hex8/16/32/64(...)`, `con_dec32(u32)` — number formatting, no
  buffering beyond a 10-byte stack scratch buffer in `con_dec32`.
- `con_getc(void)` — blocking single-character read; named keys are expanded
  to their VT100 escape sequence and drip-fed one byte per call via the
  `pend` pointer, so a keyboard `K_UP` and a serial `ESC [ A` look identical
  to every caller.
- `con_getkey(void)` — blocking read of one *key*: ASCII below 0x80, or a
  `K_*` code from kbd.h, no VT100 translation. `con_getc` is built on top of
  this.
- `con_serial_present(void)` — did the COM1 loopback test at init find a real
  UART? (`v` command and boot banner use this.)
- `con_serial_ready(void)` / `con_serial_getc(void)` — non-blocking serial
  poll / raw byte read, used by `kr` (raw scancode monitor) to detect "any
  serial byte" as an exit condition.

## Key structures / state

- `cur_row`, `cur_col` — cursor position, the only screen state kept in
  software (VGA hardware cursor is a mirror, updated via `vga_move_cursor`).
- `com1_ok` — set once by the loopback test in `con_init`; every serial path
  gates on this so an absent UART (which reads back `0xFF` everywhere,
  console.c:61) is never mistaken for "byte always ready."
- `pend` — static pointer into a `key_seq()` string, the drip-feed state for
  multi-byte VT100 sequences returned by `con_getc`. Global, single-threaded
  by construction (no concurrent readers are possible in this kernel).

## Dependencies

Depends only on `io.h` (port + MMIO primitives) and `kbd.h` (for `kbd_poll`
and the `K_*` enum). Everything else in the kernel depends on this file for
all user-visible output and all blocking input — `main.c`'s entire REPL loop
is `con_getc`/`exec`.

## Design notes worth keeping

- The loopback self-test (console.c:59-65) is the only reliable way to tell
  "real UART present" from "floating bus reading 0xFF," and 0xFF happens to
  alias with "RX ready" (bit 0 set) forever, which would otherwise hang
  every blocking read. Comment at console.c:59-61 spells this out.
- `con_putc`'s serial write has a bounded spin-wait (100000 iterations,
  console.c:86) specifically so a half-dead UART can't hang the whole
  monitor — a deliberate finite timeout rather than an infinite poll.
- `con_reset_cursor()` vs `con_init()` (console.h:24-25, console.c:72-80) is
  a one-function fix for a real bug class: naively calling `con_init()` to
  "fix the screen" after a video mode change silently eats serial input that
  was already queued, because the loopback test reprograms MCR and does a
  round-trip byte through the port.
- Named-key VT100 mapping (`key_seq`, console.c:177-192) is intentionally
  partial — only cursor/nav keys get sequences; function keys, GUI keys, and
  lock keys have no textual form and are silently dropped by `con_getc`
  (they're still visible via `con_getkey` for callers that want raw `K_*`).

## Messiness / review notes

- VGA and serial output are unconditionally coupled inside `con_putc`
  (console.c:82-111): every character always goes to both, there is no way
  to mute one without editing this function. That's fine for a
  single-purpose debug console, but it means "serial-only" or "VGA-only"
  output (e.g. for a future non-interactive/headless boot path) isn't
  expressible without a new parameter or a global flag.
- `con_init()` does two unrelated jobs under one name: (1) reset the VGA
  screen state, (2) program and self-test COM1, (3) drain a stray 8042
  byte. The existence of `con_reset_cursor()` as a workaround for job (1)
  needing to run without jobs (2)/(3) suggests these three were already
  outgrowing a single entry point — the split just hasn't been carried all
  the way (e.g. a `con_serial_init()` separate from `con_vga_reset()`).
  The 8042 drain buried at the end of `con_init` (console.c:67-69) is also
  arguably a keyboard-driver concern living in the console file.
- `con_getc`/`con_getkey`/`pend` (console.c:194-237): the drip-feed state
  machine for VT100 sequences is a small amount of genuinely fiddly logic
  (`pend` global, re-entered on every call) for a fairly narrow feature —
  worth deciding whether that complexity earns its keep versus, e.g., a
  small ring buffer or just leaving escape-sequence expansion to callers
  that actually need it (only the monitor's line editor does).
- No header/implementation separation for "policy" bits: `con_puts_pad`
  (table formatting) sits at the same level as `con_hex32` (primitive
  formatting) and `con_getc` (blocking I/O) — three different layers of
  abstraction in one flat file with no internal section markers beyond
  comments.

## Open questions for the rewrite

- Should VGA output and serial mirroring be decoupled (two independent sinks
  a caller can select) or is "always both" acceptable permanently for a
  debug-only console?
- Does `con_init`'s three-jobs-in-one get split into named sub-inits
  (`con_vga_init`/`con_serial_init`/`con_kbd_drain`), with `con_init` as a
  thin wrapper, so `con_reset_cursor()` stops being a special case?
- Is the VT100 drip-feed (`pend`, `key_seq`) worth keeping as-is, simplifying
  to a fixed-size ring buffer, or dropping from `con_getc` entirely and
  pushing escape-sequence policy up to the one caller (the line editor) that
  wants it?
- Any plan for interrupt-driven serial/keyboard I/O, or is polling-only a
  permanent design constraint given the "no paging, no scheduler" philosophy
  of the rest of the kernel?
