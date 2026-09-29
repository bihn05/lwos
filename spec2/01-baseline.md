# 01 — v2 现状快照（2026-09-30）

这一篇只记录事实，不做规划。后续文档里的"现状"都以这里为起点。
每做完一个大阶段，就在这篇末尾追加一段新的快照，旧的不删。

## 启动链

```
MBR (0x7C00) → STAGE2 (0x900, 实模式)
   ├─ 收集 E820 → 0x8000 (MMAP)
   ├─ 收集 VBE/EDID/显卡 PCI 信息 → 0x9000 (BINFO)
   ├─ 留下 BIOS 跳板 → 0xA000 (TRAMP_BLK)
   └─ 读 FAT32 找 LOADER.BIN → 0x10000，进保护模式
LOADER.BIN (0x10000，自带 PIO + 只读 FAT32，不依赖 lib)
   └─ 按 BOOT.INI：LOAD ABI.BIN→100000H，LOAD MONITOR.BIN→200000H，
      ATTACH 100000H，PORT 200000H
ABI.BIN (0x100000，库映像，无栈)
MONITOR.BIN (0x200000，调试器 / 命令行)
```

## 段与中断

- GDT 在 `boot/stage2.s:gdt_start`，5 项（limit 0x27）：NULL、32 位平坦代码 0x08、32 位平坦数据 0x10、16 位代码、16 位数据（给 BIOS 跳板用）。**没有 TSS 描述符**。
- IDT 在 `app/monitor/src/idt.c:idt`，48 个门，**由 monitor 拥有**。ISR 桩在 `app/monitor/src/isr.s`。
- PIC **没有重映射**；`idt_init` 把两片 PIC 全部屏蔽（`0x21`/`0xA1` 写 `0xFF`）。
- 所有设备都是**轮询**：键盘（`kbd_poll`）、ATA PIO。没有定时器中断。

## ABI 表（`lib/include/abi.h`）

| 段 | slot | 内容 |
|---|---|---|
| 映像头 | 0x00–0x04 | magic / entry / bss / stack |
| 控制台 | 0x10–0x1C | putc、puts、put_*、clear、dump128、getk/getc/getp |
| IO | 0x20–0x2B | in/out b/w/l、kbd probe/enable、gfx enter/exit/live、fpu_init |
| 信息 | 0x30–0x34 | fb 地址、宽、高、bpp、pitch |
| PCI | 0x40–0x42 | present、read_dword、write_dword |
| 保留 | 0x7F | 目前指向 `binfo_dump` |

ABI.BIN 里链接了 `ata.c`，但**没有 ATA 的 slot**，外部用不了。

## 编译分层（`makefile`）

- **KERN**（loader、lib/abi、以后的 resman）：禁止浮点，不链 libgcc。
- **APP**（app/*）：允许 x87，链 libgcc。

## 已有但未接入的东西

- `app/resman/`：只有空的 `resman.h` 和 `resman.c`。
- `tools/mkexe/doc.txt`：LWP 可执行格式的头部草稿（见 [40-lwp-exe.md](40-lwp-exe.md)）。
- 根目录下还有一个空的 `resman/`，和 `app/resman/` 重复，应该删掉其中一个。

## 模拟器

- `bochsrc`：i440fx，32MB 内存，只有 ata0，PS/2 鼠标关闭，**没有网卡，没有 AHCI**。
  Bochs 本身不模拟 AHCI；AHCI 开发要用 QEMU（`-machine q35` 或 `-device ahci`）。
