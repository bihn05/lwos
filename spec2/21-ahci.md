# 21 — AHCI

## 目标

通过 AHCI 读写 SATA 盘，对上层表现为一个 `BLKDEV` 设备对象（[22](22-blockdev-fs.md)），
和 PIO 的 ATA 驱动可以互换。

## 现状

- `lib/abi/src/ata.c`：legacy PIO，固定端口 0x1F0/0x170。已链进 ABI.BIN，但**没有 slot**。
- 没有 AHCI 代码。
- **Bochs 不模拟 AHCI**。开发和测试要用 QEMU：`-machine q35`（自带 ICH9 AHCI）或 `-device ahci`。

## 依赖

- 前置：[20 PCI 枚举](20-pci-enum.md)（找控制器、拿 BAR5）、[50 resman 内存](50-resman-memory.md)（对齐的 DMA 缓冲）。
  在 resman 做好之前，可以先用链接脚本里一块静态对齐的 bss 顶替。
- 后继：[22 块设备](22-blockdev-fs.md)。

## 设计

### 文件

```
lib/abi/include/ahci.h    HBA 和端口寄存器、FIS、命令头、PRDT 结构
lib/abi/src/ahci.c        驱动；在 ABI 内部驱动表里登记 class 01/06/01
```

### 寄存器（AHCI 1.3 规范）

HBA 寄存器在 **BAR5（ABAR）**，是 MMIO：

| 偏移 | 名字 | 用途 |
|---|---|---|
| 0x00 | CAP | 端口数、命令槽数、是否支持 64 位 |
| 0x04 | GHC | bit31 AE（AHCI 使能）、bit1 IE、bit0 HR（复位） |
| 0x0C | PI | 哪些端口实现了（位图） |
| 0x10 | VS | 版本 |
| 0x24 | CAP2 | bit0 BOH：是否支持 BIOS 交接 |
| 0x28 | BOHC | BIOS/OS 交接控制 |

端口 n 的寄存器在 `0x100 + 0x80 × n`：

| 偏移 | 名字 | 用途 |
|---|---|---|
| 0x00/0x04 | PxCLB/PxCLBU | 命令列表基址（1KB 对齐） |
| 0x08/0x0C | PxFB/PxFBU | 接收 FIS 基址（256B 对齐） |
| 0x10 | PxIS | 中断状态（写 1 清） |
| 0x18 | PxCMD | bit0 ST、bit4 FRE、bit14 FR、bit15 CR |
| 0x20 | PxTFD | 任务文件：状态和错误 |
| 0x24 | PxSIG | 签名：0x00000101 为 SATA 盘，0xEB140101 为 ATAPI |
| 0x28 | PxSSTS | DET（低 4 位）= 3 表示有设备且链路建立 |
| 0x30 | PxSERR | 错误（写 1 清） |
| 0x38 | PxCI | 命令发出位图 |

**用 `volatile DWORD *` 访问 MMIO**，每次读写都必须真正落到硬件上。

### 初始化顺序

1. PCI 命令寄存器置 Memory Space + Bus Master。
2. 如果 CAP2.BOH = 1，做 BIOS 交接：置 BOHC.OOS，等 BOHC.BOS 清零。
3. GHC.AE = 1。
4. 对每个在 PI 里的端口：
   1. 停端口：清 ST，等 CR 清零；清 FRE，等 FR 清零。
   2. 填 PxCLB / PxFB（高 32 位写 0）。
   3. 清 PxSERR、PxIS。
   4. 置 FRE，再置 ST。
   5. 读 PxSSTS.DET 和 PxSIG，判断有没有盘、是什么盘。
5. 对每个 SATA 盘发 IDENTIFY DEVICE（0xEC），解析扇区数。解析函数可以复用 `ata.c:ata_identify_parse`。

### 发一条命令（轮询版）

1. 选一个空闲槽（第一版固定用槽 0）。
2. 填命令头：CFL = 5（FIS 长度 5 个 DWORD），W 位（写操作时），PRDTL = PRDT 项数。
3. 填命令表：H2D 寄存器 FIS（类型 0x27，C=1，命令码，LBA48，扇区数）；PRDT 每项最多 4MB。
4. 等 PxTFD 的 BSY 和 DRQ 都清零 → 置 PxCI 的对应位。
5. 轮询到 PxCI 该位清零为止；检查 PxIS.TFES 和 PxTFD.ERR。

命令：READ DMA EXT（0x25）、WRITE DMA EXT（0x35）、FLUSH CACHE EXT（0xEA）。

### 内存

每个端口需要 1KB 命令列表 + 256B 接收 FIS + 至少一张命令表。
按 [02](02-decisions.md) D2，这块内存由调用方通过 `lw_drv_bind` 的 `dma` 参数传进来，驱动自己切分。
一个端口一张命令表（8 个 PRDT 项）的话，大约 1.5KB/端口。

### 和 v1 的关系

v1 的做法（AHCI 优先、失败回退 PIO、共用一个设备结构）作为先例保留：
AHCI 和 PIO 都导出 `BLKDEV`，上层不关心底下是哪一个。

## 里程碑

| # | 内容 | demo / 验收 |
|---|---|---|
| A1 | 找到控制器，读 CAP/PI/VS，列端口 | QEMU q35：打印版本 1.0 或 1.3、端口位图、每个端口的 DET 和 SIG |
| A2 | 端口初始化 + IDENTIFY | 打印型号、序列号、扇区数，和 PIO 版读到的一致 |
| A3 | 读扇区 | 读 LBA 0，和 PIO 读的逐字节比对，末尾应是 0x55AA |
| A4 | 写扇区 + flush | 往镜像末尾的空闲扇区写一段图案，重启后读回校验 |
| A5 | 包成 `BLKDEV`，登记进驱动表 | resman 扫描后设备表里出现 `blk0` |

## 待决

- 第一版只用槽 0、一次一条命令。以后要不要多槽并发？对 CNC 场景大概不需要。
- 真机上的 BIOS 可能把 SATA 设成 IDE 兼容模式，这时 class 是 01/01 而不是 01/06。要不要在 PIO 驱动里也处理"原生模式 IDE"（端口号来自 BAR0–3 而不是固定的 0x1F0）？
