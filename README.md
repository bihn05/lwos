# LWOS

**i hand/manually wrote everything ...**

A hobby operating system with an IBM-PC-inspired look and feel, aimed at
CNC (computer numerical control) milling devices.

Note: LWOS is *stylistically* IBM-PC-like, but it does **not** depend on the
BIOS. Firmware support (custom BIOS, IBM-compatible machines) lives in a
separate project and is out of scope here.

> **Status:** early, under active refactor. The repository is being rebuilt
> and standardized from scratch, so APIs, layout, and on-disk formats may
> change without notice.

## What works today

- **MBR** — a Master Boot Record that boots the system.
- **FAT32 (read-only)** — minimal FAT32 reading.
- **`tools/mkfat`** — a small C utility for creating FAT images.

Everything else (shell, utilities, CNC stack) is planned but not yet present.

## Planned

- `DEBUG` — a debugger modeled after MS-DOS `DEBUG.COM`
- `EDIT` — a text editor modeled after MS-DOS `EDIT.COM`
- Documentation for design, memory layout, and CNC routing

## Repository Layout

    boot/       MBR and early boot code
    fsroot/     FAT32 (read-only) implementation
    tools/      Host-side utilities (e.g. mkfat)
    report/     review notes
    SPEC/       Design and route documentation

## Building and Running

Requirements:

- A Unix-like environment with `make`
- A C compiler (for `tools/`)
- [QEMU](https://www.qemu.org/) or [Bochs](https://bochs.sourceforge.io/)

Build and boot in the emulator:

```sh
make run