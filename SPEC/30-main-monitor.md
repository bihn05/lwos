# Kernel entry point + hand-typed monitor

**Files:** kernel/main.c (1063 lines)
**Status:** built by makefile (`OBJS` includes `kernel/main.o`, last object in the link)

## Purpose

`kernel/main.c` is two things stapled together: `kmain()`, the kernel's actual
entry point (called once, after `stage2` has copied the kernel to 1MB and
jumped into protected mode), and `exec()`, a DEBUG.COM-style interactive
monitor that is the only user interface this OS has. There is no shell, no
process model, no scheduler — booting this image drops you into a REPL that
reads one line at a time from console input (keyboard or COM1, see
[[31-console]]) and dispatches on the first one or two characters.

The monitor exists because the user's practice goal is to write and debug
drivers by hand-typing assembly and memory writes into a running machine
(see the ABI table contract, [[04-kernel-abi]]) rather than editing source and
rebuilding. So this file is simultaneously "the kernel" and "the debugger for
whatever the kernel doesn't do yet" — every subsystem (ATA, PCI, keyboard,
filesystem, graphics trampoline) gets a monitor command before or instead of
getting a real caller.

## Public interface

`kmain(void)` — entry point, called from the trampoline in the `.stage2`
domain once protected mode is live. Never returns.

### Monitor commands (dispatched by `exec()`, main.c:794-1014)

First char (`c0`) and optional second char (`c1`) select the command; the rest
of the line is passed to a handler that parses hex/word/path tokens.

| cmd | args | does |
|---|---|---|
| `h`, `h1`/`h2`/`h3` | — | paged help (3 screens, main.c:712-765) |
| `d` | `<addr> [len]` | dump memory bytes (default len 80) |
| `e` | `<addr> <b>...` or `"str"` | write bytes; supports quoted strings with `\n\r\t\0\e\\\"` escapes |
| `r` | — or `<reg> <val>` | show/set debuggee register (eax..edi, eip, efl, ds/es/fs/gs) |
| `rs` | — | live CPU sysregs: GDTR/IDTR/LDTR/TR/CR0-4/EFLAGS (read-only) |
| `m` | — | dump E820 memory map |
| `md` | `<addr> [n]` | dump n dwords (MMIO, alignment-enforced) |
| `mw` | `<addr> <v>...` | write dwords, read back |
| `z` | — | reset debuggee context (`ctx_reset`) |
| `g` | — | run debuggee until fault/breakpoint (`dbg_enter`) |
| `t` | `[n]` | single-step n instructions |
| `i`/`iw`/`id` | `<port>` | port read (byte/word/dword) |
| `o`/`ow`/`od` | `<port> <v>` | port write |
| `q` | — | reboot via 0xCF9 |
| `p` | — or `<bus> <dev> <fn>` | PCI enumerate or one function's detail |
| `ad` | — | probe ATA devices (`ata_detect`) |
| `x` | — | dump the ABI table (`abi_dump`) |
| `xb` | — | dump the boot info block (`binfo_dump`) |
| `l`/`w`/`W` | `<addr> <dev> <lba> <n>` | disk read / write-with-confirm / write-no-confirm |
| `sl` | — | list FAT32 root dir (`fs_dump`) |
| `sc` | `<path> <addr> <n>` | create file from n sectors |
| `sw` | `<path> <addr> <n>` | replace file content (asks) |
| `sr` | `<path> <addr> [n]` | read file into memory, n sectors or whole file |
| `k` | — | probe 8042 keyboard controller |
| `ke` | — | enable keyboard clock + scanning |
| `kr` | — | raw scancode monitor |
| `v` | — | graphics trampoline status |
| `ve`/`vt` | — | enter VBE mode (`vt` = timed round trip back to text) |
| `vx` | — | force text mode restore |
| `f`/`F` | `<addr> <len> [byte]` | fill memory; `f` asks before touching live ranges, `F` doesn't |

Two commands are asymmetric on purpose and documented as such in-file: `w`/`W`
mirror `f`/`F` (ask vs. don't), and `sc`/`sw` mirror that same "create can't
clobber, replace can and asks" split (main.c:595-600).

## Key structures / state

- `struct ctx uctx` (main.c:16) — the debuggee's saved register state, the
  thing `g`/`t`/`r`/`z` all operate on. Distinct from the live CPU state that
  `rs` reads directly via `sgdt`/`sidt`/`mov %crN` — see the comment at
  main.c:104-113 explaining why `rs` is deliberately read-only (writing CR0
  from the prompt would clear PE under the monitor's own feet).
- `static u8 dbg_stack[4096]` — the debuggee's stack, separate from the
  monitor's own stack.
- `static char line[128]` — the one-line input buffer; `prompt_read()` is the
  only line editor (backspace only, no history).
- `fill_hits_live()` (main.c:349-362) — a hardcoded list of "ranges that are
  still live after boot": the stub+GDT (`__kernel_lma`), the kernel itself
  (`__bss_end`), the E820 buffer (`MMAP_BUF`), and the boot info block
  (`BINFO_BASE`). This is the one place main.c encodes cross-cutting layout
  knowledge instead of asking each owner.

## Dependencies

Pulls in and drives every other kernel unit: `console.h` (all I/O),
`ctx.h`/isr.s (execution control), `kbd.h`, `mmap.h`, `ata.h`, `pci.h`,
`abi.h`, `binfo.h`, `tramp.h`, `fs.h`. `kmain()`'s boot sequence
(main.c:1016-1063) is the de facto init order for the whole kernel:
`con_init` → `idt_init` → `mmap_init` (before anything reuses the 0x8000
buffer) → `ctx_reset` → `kbd_enable` → `kbd_probe` → `ata_detect` →
`fs_init(0)` → `help(1)` → REPL loop. Nothing else defines this order; a
rewrite that wants a different init sequence has to know it currently lives
here as straight-line code with one inline comment each.

## Design notes worth keeping

- The byte-vs-dword split (`d`/`e` vs `md`/`mw`) is a real hardware
  constraint, not a style choice: gen3 GPU register files decode per-dword
  and a byte access "returns garbage on some offsets and wedges the chip on
  others" (main.c:263-269). Alignment is enforced, not rounded, deliberately.
- `report_stop()`/`cmd_go`/`cmd_trace` distinguish INT1 (single-step/trace)
  and INT3 (breakpoint via a planted `0xcc`) from a real fault, and
  clear `EFL_TF` before reporting so a stale trace flag doesn't show as
  live (main.c:691-693).
- `v`/`vt` command ordering advice (main.c:851-853) is a safety note:
  reach for `vt` before `ve` because `vt` returns on its own, so a broken
  video mode can't strand you with no console and no way to type the fix.
- `vt`'s "wait" is a busy loop, deliberately not `io_wait()`, because a port
  write traps to the emulator and would make the round trip take tens of
  seconds under QEMU (main.c:878-887).

## Messiness / review notes

- **main.c is doing three jobs**: kernel entry/init (`kmain`), a generic
  low-level monitor (memory/register/port primitives), and a set of
  per-driver command wrappers (ATA, PCI, FAT32, keyboard, graphics) that
  really belong next to their drivers. At 1063 lines it is by far the
  largest file in the kernel and the only one without a header — everything
  in it is `static` except `kmain`. A split along those three seams (core
  monitor loop / debuggee execution / per-device command table) is the
  obvious candidate, and would also let each driver register its own
  commands instead of `exec()`'s single giant switch knowing about all of
  them.
- `exec()`'s dispatch (main.c:794-1014) is a hand-rolled switch on `c0`/`c1`
  with no table — adding a command means adding a `case` and updating
  `help()`'s hardcoded text separately (main.c:712-765). The two are already
  slightly coupled by convention only; nothing checks that every case in
  `exec()` has a matching help line or vice versa. Worth a design decision:
  keep the switch (simple, matches "hand-typed, predictable" philosophy) or
  move to a command table that generates help text (less duplication, more
  indirection).
- `help()` is paginated to fit a 25-line VGA screen (`HELP_PAGES = 3`,
  main.c:709-765) — this is a UI constraint (80x25 text mode) baked directly
  into monitor logic. If the console ever grows scrollback or a different
  output width, this paging scheme has no way to know.
- `cmd_disk`/`cmd_file_write`/`cmd_file_read` (main.c:509-652) are close to
  identical in shape (parse args → print intent → confirm if destructive →
  call driver → print result/error) but are not factored through a shared
  helper; `confirm()` is shared, the rest isn't.
- `fill_hits_live()` hardcodes four independently-owned memory ranges in one
  function outside any of the four owners. It's honest about being coarse
  ("whole page, not the exact ARDS extent", main.c:355-356) but it is a
  layering violation by construction: main.c has to know internal layout
  facts about mmap.c and binfo.c to protect them from its own `f` command.
- Two register naming schemes coexist: `uctx` fields use `eax`/`efl`/etc.
  lowercase struct member names (ctx.h), while `reg_slot()` (main.c:242-259)
  maps user-typed lowercase strings 1:1 onto them — fine today, but any
  future register (e.g. exposing `cr3` as settable) would need a decision
  about whether "settable via `r`" and "member of `struct ctx`" stay the
  same set.
- `fb_gradient()` (main.c:22-40) is test/demo code (paints a diagonal
  gradient to prove the framebuffer is live) sitting in the same file and at
  the same level as the monitor's actual I/O primitives, with no marker
  distinguishing "permanent driver-adjacent code" from "a one-off visual
  smoke test."

## Open questions for the rewrite

- Does the monitor stay one big command switch, or become a registration
  table that each driver populates (and if so, who owns `help()`'s text)?
- Should driver-specific commands (`ad`, `p`, `k`/`ke`/`kr`, `sl`/`sc`/`sw`/`sr`,
  `v`/`ve`/`vt`/`vx`) move into their respective driver files, leaving
  main.c with only the generic primitives (`d`/`e`/`m`/`md`/`mw`/`r`/`rs`/`g`/`t`/
  `i`/`o`/`f`/`F`)?
- Is `fill_hits_live()`'s hardcoded live-range list acceptable permanently
  (small, rarely-touched, one file to check), or should each owner (mmap,
  binfo, the stub/GDT region) publish its own range so main.c doesn't need
  to know their internals?
- `kmain()`'s init order is currently implicit in straight-line code with
  scattered comments explaining ordering constraints (e.g. "before anything
  reuses the low buffer at 0x8000"). Worth turning into an explicit,
  self-documenting init table, or is straight-line code with comments
  preferred for a hand-typed-debugging-focused OS where you want to *read*
  the boot sequence top to bottom?
- `fb_gradient()`/demo code: keep inline as a quick smoke test, or move out
  to make main.c purely "monitor + entry," with demos living elsewhere?
