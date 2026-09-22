# Keyboard (8042 + Set-1 Decoder)

**Files:** kernel/kbd.c (470 lines), kernel/kbd.h (48 lines)
**Status:** built by makefile (`OBJS` includes `kernel/kbd.o`)

## Purpose

Two mostly-independent halves in one file. The first talks to the 8042
keyboard controller at ports 0x60/0x64 for diagnostics and enabling: probing
whether a controller is even present, self-testing it, reading/writing its
config byte, and forcing the keyboard clock on when firmware/EC left it
disabled (kbd.c:1-6 explains why this matters: "A USB keyboard only reaches
ports 0x60/0x64 if the firmware's USB Legacy Support traps them from SMM.
Many newer boards have no 8042 at all, or ship with legacy emulation off.").
The second half is `kbd_feed()`, a pure scancode-set-1 state machine that
turns raw bytes from the controller into ASCII characters or `K_*` codes,
independent of any I/O.

## Public interface

- `void kbd_probe(void)` — monitor diagnostic: dumps status register,
  self-test (0xAA→0x55), config byte with each bit named, device echo
  (0xEE→0xEE) to confirm a keyboard is physically attached.
- `int kbd_enable(void)` — clears the keyboard-clock-disable config bit,
  verifies it, sends 0xF4 (enable scanning) to the device and waits for the
  0xFA ACK. Returns 1 on success.
- `void kbd_raw(void)` — prints every status/scancode byte as it arrives;
  exits on ESC (from the keyboard) or 'x'/ESC on the monitor's serial
  input.
- `int kbd_feed(u8 sc)` — feed one raw scancode byte; returns a `K_*` code
  or ASCII char on a completed keypress, `K_NONE` while a multi-byte
  sequence (0xE0/0xE1 prefixes) is still in progress. **Pure function**: no
  I/O, only touches the static decoder state below.
- `int kbd_poll(void)` — non-blocking: reads status, checks OBF set and
  AUXB clear (a real keyboard byte, not mouse data), and if so pulls the
  byte and feeds it to `kbd_feed()`. `K_NONE` if idle.
- `u8 kbd_mods(void)` — currently-held modifier bits (`KM_*`).
- `void kbd_reset_state(void)` — clears modifiers, lock states, and any
  pending 0xE0/0xE1 prefix; also pushes the new (all-off) lock state to the
  LEDs.

`K_*` constants for non-ASCII keys start at 0x100 (kbd.h:16-25) specifically
so a caller wanting only text can test `k < 0x80` and ignore the rest
(kbd.h:14-15).

## Key structures / state

- Four lookup tables, all designated-initializer arrays "on purpose ...
  [because] a flat table silently shifts every entry that follows a hole"
  (kbd.c:274-276):
  - `map_lo[0x59]` — unshifted char per make code.
  - `map_hi[0x59]` — shifted char, only where it differs from `map_lo`; 0
    means "use `map_lo`, upcased if a letter."
  - `map_fn[0x59]` — non-ASCII keys in the main block (F-keys, nav cluster
    with numlock off).
  - `map_e0[0x80]` — keys reached only through the 0xE0 prefix (arrow
    cluster, right ctrl/alt, keypad `/`, Windows/menu keys).
- Decoder state, all file-static: `mods` (KM_* bits held), `caps`/`num`/
  `scroll` (lock states), `e0_pending` (saw 0xE0, next byte is extended),
  `e1_skip` (bytes still to swallow for a Pause sequence) — kbd.c:330-333.
- Port/status-bit macros for the controller (kbd.c:10-22): `KBD_DATA`/
  `KBD_STAT`/`KBD_CMD` all resolve to 0x60/0x64; status bits `ST_OBF`
  (byte ready), `ST_IBF` (controller busy), `ST_AUXB` (byte is mouse data,
  not keyboard), etc.

## Dependencies

- Includes console.h for all diagnostic/raw-mode output and
  `con_serial_ready`/`con_serial_getc` (used by `kbd_raw()`'s exit
  condition).
- Nothing in this file depends on ata.c/ahci.c/pci.c. `kbd_poll()`/
  `kbd_feed()` are presumably called from kernel/main.c's monitor input
  loop — not verified in this pass.

## Design notes worth keeping

- **Set-1 layout rationale**, kbd.c:266-275: make codes run 0x01-0x58 for
  the main block; break code = make code with bit 7 set. Keys IBM added
  later (arrow cluster, right ctrl/alt, keypad `/`) are reached through an
  0xE0 prefix and *reuse a main-block make code*, which is exactly why the
  prefix has to be carried as decoder state across calls rather than decoded
  in one shot. Pause is the outlier: 0xE1 plus five more bytes, all
  swallowed (kbd.c:396-401).
- **Numlock/shift keypad interaction mirrors BIOS behavior**: "The keypad
  splits on numlock: digits with it on, navigation with it off. Shift
  inverts that, the way a PC BIOS does." (kbd.c:424-426) — implemented as an
  XOR of `num` and shift-held (kbd.c:428).
- **Shift/caps interaction is letters-only**: "shift and caps cancel out,
  and only on letters" (kbd.c:444-445) — an XOR, applied only in the
  `a`-`z` branch, not to punctuation.
- **Ctrl-letter and ctrl-punctuation both produce control codes**
  (kbd.c:447-456): letters mask to `c & 0x1F` (^A..^Z); a fixed set of
  punctuation (`[`, `\`, `]`, backtick, `-`) maps to specific control codes
  (27, 28, 29, 0, 31) matching what a real terminal driver expects.
- **`kbd_probe()`'s EC-quirk workaround is explained inline, not just
  coded**: on a laptop whose internal keyboard was physically removed, "the
  EC often disables the keyboard clock at POST" but an external USB
  keyboard still gets through via legacy emulation once scanning is
  re-enabled (kbd.c:150-153). `kbd_enable()` goes further: after clearing
  the clock-disable bit, if bit 4 *still* reads disabled it presses on
  anyway rather than bailing, because "on a laptop whose internal keyboard
  was removed, the EC reports the interface as disabled while still
  forwarding scancodes to 0x60 ... let the 0xF4 ACK below decide" (kbd.c:
  187-193). This is hardware-observed behavior baked into control flow, not
  a hypothetical.
- **`set_leds()` is deliberately best-effort**: "a controller that will not
  answer is not worth stalling the decoder over" (kbd.c:348-349) — every
  wait in it can fail silently and the function just returns.
- **`kbd_raw()` drains stale serial input before its loop** specifically
  because leftover bytes from the command line that invoked it would
  otherwise make the loop exit immediately (kbd.c:230-231).

## Messiness / review notes

- **Two unrelated concerns share one file with no internal boundary
  marker beyond a comment** (`/* ---- scancode set 1 decoder ---- */` at
  kbd.c:266): 8042 controller I/O (`kbd_probe`, `kbd_enable`, `kbd_raw`,
  `wait_write`/`wait_read`/`read_config`/`write_config`) versus the pure
  decoder (`kbd_feed` and its tables). This is the file in this batch with
  the clearest case for a split — `kbd_feed()`'s host-testability (see
  below) is actively *reduced* by living in the same translation unit as
  code that calls `inb`/`outb` directly, since a test harness has to stub
  those symbols out even though the decoder itself never calls them.
- **`show_status()` and `kbd_probe()`'s bit-by-bit printing duplicate the
  same "print flag N, on/off label" pattern six-plus times each**
  (kbd.c:44-56, 96-111) with no shared helper — straightforward to factor
  into a table-driven printer if that reduces noise during the rewrite.
- **`set_mod`'s bit-clear cast** (`mods &= (u8)~bit`, kbd.c:367) and the
  identical pattern in `kbd_enable` (`c &= (u8)~0x10`, kbd.c:174) show the
  same "clear a bit in a `u8` without a compiler warning from integer
  promotion" idiom repeated at two call sites with no shared macro — minor,
  but a candidate for a `BIT_CLR`-style helper if the codebase gains one
  elsewhere.
- No evidence of duplicated/competing implementations analogous to
  ata.c vs. external/ata.c — this file appears to be original, not ported.

## Host-testability (validated, not hypothetical)

`kbd_feed()` is pure state-machine code: no I/O, only the file-static
decoder state. The kbd-decoder-testing project memory documents that this
has already been exploited successfully: copy kbd.c/kbd.h to a scratch
directory alongside stub `io.h` (inb/outb as no-ops) and `console.h`, then
drive `kbd_feed()` directly with scancode byte arrays — covering the tricky
cases (0xE0 extended codes, the Pause sequence, numlock/shift interaction)
without needing hardware or even a boot. The same document notes
`kernel/ata.c` is amenable to an analogous trick (stub `outb`/`outw`/`inb`/
`inw` as a fake drive). If the rewrite splits controller-I/O from decoder
logic (see Messiness above), this host-harness approach becomes trivially
easier to keep working, since the decoder file would no longer need any
stub headers at all.

## Open questions for the rewrite

- Split 8042 controller I/O (`kbd_probe`/`kbd_enable`/`kbd_raw`/the
  `wait_*`/`*_config` helpers) from the pure decoder (`kbd_feed` and its
  tables) into two files? The existing host-test harness is already
  exploiting the decoder's purity; a physical file split would make that
  boundary structural instead of just behavioral.
- Should the four scancode tables (`map_lo/hi/fn/e0`) move to a
  data-driven format (e.g. a single table of `{make, unshifted, shifted,
  fn_code}` structs) rather than four parallel arrays indexed by the same
  code?
- Is the EC-quirk "press on even if bit4 still reads disabled" heuristic in
  `kbd_enable()` something to keep permanently, or was it a one-machine
  workaround that deserves a named flag/config option instead of being
  unconditional?
- Does `kbd_raw()` (a manual/interactive diagnostic loop) belong in the same
  module as `kbd_poll()` (the production input path), given they're used at
  very different times for very different purposes?
