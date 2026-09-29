# 10 — 物理内存布局（无分页）

## 目标

写出一张**权威的内存地图**：哪些地址被谁永久占用、哪些只在启动时用、哪些交给 resman 分配。
所有固定地址的常量都应该能在这张表里找到出处。

## 现状

固定地址散落在 `boot/stage2.s`、`lib/abi/include/binfo.h`、`lib/abi/include/tramp.h`、
各个 `.ld` 文件里，没有一张总表。

## 地图

### 低 1MB

| 起 | 止 | 谁 | 生命期 | 备注 |
|---|---|---|---|---|
| 0x00000 | 0x004FF | IVT + BDA | 永久 | BIOS 跳板需要 IVT 完好 |
| 0x00900 | ~0x01200 | STAGE2 | **永久** | `tramp_int10` 的代码在这里，跳板要用 |
| 0x07C00 | 0x07DFF | MBR | 启动时 | 之后可回收 |
| 0x08000 | 0x08A1F | MMAP（E820）+ stage2 的 FAT 变量 | resman 拷走前 | resman 初始化时拷到自己的 bss |
| 0x09000 | 0x090E3 | BINFO | 永久（只读） | |
| 0x09100 | 0x09FFF | VBE 临时块 / VBE 栈 | 启动时 | |
| 0x0A000 | 0x0A0FF | TRAMP_BLK | 永久 | 跳板参数块 |
| ~0x0B000 | 0x0C000 | 跳板栈（`TRAMP_STACK_TOP`） | 永久 | |
| 0x10000 | ~0x12000 | LOADER.BIN | 交接前 | 之后可回收 |
| ~0x80000 | 0x9FFFF | **EBDA 可能在这里** | 永久 | 起点看 BDA 的 `[0x40E]`，不同机器不同 |
| 0xA0000 | 0xFFFFF | 显存窗口 / ROM | — | |

> **已知隐患**：`boot/stage2.s:pm_entry` 把 `esp` 设为 `0xA0000`，loader 在这个栈上运行。
> 栈往下长，第一次 push 就写到 0x9FFFC，也就是**大多数机器 EBDA 的位置**（常见起点
> 0x9FC00，有些笔记本 EBDA 更大）。Bochs 下没事，真机上可能踩坏 EBDA，进而导致之后的
> BIOS 跳板（INT 10h）行为异常。建议把 loader 栈放到 0x90000，或者按 `[0x40E]` 算出
> EBDA 起点再往下留余量。

### 1MB 以上

| 起 | 止 | 谁 | 备注 |
|---|---|---|---|
| 0x100000 | 0x17FFFF | ABI.BIN（512KB 预留） | 现在约 11KB |
| 0x180000 | 0x1FFFFF | RESMAN.BIN（512KB 预留） | 见 [02](02-decisions.md) D2 |
| 0x200000 | 0x2FFFFF | MONITOR.BIN（1MB 预留） | |
| 0x300000 | E820 上限 | **resman 堆** | EXE、DLL、任务栈、DMA 缓冲、GUI 后备缓冲都从这里分 |
| fb_phys | +pitch×h | 线性帧缓冲 | 在 RAM 之外（Bochs 通常是 0xE0000000） |
| PCI BAR | | 设备 MMIO | AHCI 的 ABAR、网卡 BAR0 |

**每个固定映像的链接脚本都应加 ASSERT**，确保映像末尾（含 bss 和栈）不越过下一段的起点。
`app/monitor/monitor.ld` 已经在检查起点，补一个终点检查即可。

## DMA 约束（给 resman 分配器的需求）

无分页 + 平坦段，所以 **C 指针的值 = 物理地址**，可以直接填进描述符。分配器只需满足：

| 用户 | 对齐 | 大小 | 其他 |
|---|---|---|---|
| AHCI 命令列表 | 1KB | 1KB/端口 | |
| AHCI 接收 FIS | 256B | 256B/端口 | |
| AHCI 命令表 | 128B | 128B + 16B×PRDT 项数 | |
| e1000 描述符环 | 16B（建议 128B） | 环长度必须是 128B 的倍数 | |
| e1000 收发缓冲 | 16B（建议 2KB） | 2KB×环长度 | |
| GUI 后备缓冲 | 16B | 1280×1024×4 = 5MB | Bochs 只配了 32MB，够用但要算着花 |

所有 DMA 地址都低于 4GB（32 位系统天然满足），不需要 64 位地址字段。

## 待决

- ABI、resman、monitor 预留的大小要不要现在就收紧？现在留得很宽，是因为不知道驱动会长多大。
- E820 报告的"可用"区域里，有没有需要避开的（ACPI 回收区在不用 ACPI 的情况下也可以当普通内存）？
