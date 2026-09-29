# 22 — 块设备抽象与运行时文件系统

你列的清单里没有这一项，但 **EXE/DLL 加载器要从磁盘读文件**，而现在运行时根本没有文件系统：
FAT32 读取只存在于 `loader/loader.c` 里，loader 交接后就死了。所以把它单独列出来。

## 目标

1. `BLKDEV`：统一的块设备接口，PIO 和 AHCI 都实现它。
2. 运行时只读 FAT32：能按路径打开文件、读到内存。以后再加写入。

## 现状

- `ata.c`（PIO）在 ABI 里，没有 slot。
- `loader.c` 里有一份只读 FAT32（单级目录、8.3 文件名），自带 PIO，不依赖 lib。

## 依赖

- 前置：PIO（已有）或 [21 AHCI](21-ahci.md)；[50 resman 内存](50-resman-memory.md)（缓冲）。
- 后继：[40 EXE](40-lwp-exe.md) 从磁盘加载。

## 设计

### BLKDEV

```c
typedef struct _BLKDEV {
    DWORD version, size;             /* 只追加字段 */
    char  name[8];                   /* "ahci0" / "ata0" */
    DWORD sector_size;               /* 512 */
    QWORD sectors;
    int  (*read) (struct _BLKDEV *d, QWORD lba, DWORD count, PVOID buf);
    int  (*write)(struct _BLKDEV *d, QWORD lba, DWORD count, const PVOID buf);
    int  (*flush)(struct _BLKDEV *d);
    void (*poll) (struct _BLKDEV *d);  /* 见 02 D4 */
    PVOID priv;                      /* 驱动私有 */
} BLKDEV, *PBLKDEV;
```

### 文件系统

- 从 `loader.c` 移植 FAT32 读取，改为通过 `BLKDEV` 读扇区。
- 在 loader 基础上加上**多级目录**（按 `/` 分段逐级查找）。
- 接口：`fs_open(path) → 句柄`、`fs_read(h, buf, n)`、`fs_size(h)`、`fs_close(h)`。
- 分区：读 MBR 分区表，挂第一个 FAT32 分区（类型 0x0B/0x0C）。

## 里程碑

| # | 内容 | demo / 验收 |
|---|---|---|
| B1 | PIO 驱动包成 `BLKDEV` | monitor 通过 `BLKDEV` 读 LBA 0 并打印 |
| B2 | FAT32 只读，根目录 | monitor 加 `dir` 命令，列出 fsroot 里的文件，和 mkfat 放进去的一致 |
| B3 | 多级目录 + `fs_read` | `type /BOOT.INI` 打印出文件内容 |
| B4 | 换成 AHCI 的 `BLKDEV` | 在 QEMU q35 下 B2/B3 结果不变 |

## 待决

- **文件系统放在哪一层？** 选项：
  1. 放进 resman（最简单，加载器就在旁边）；
  2. 单独一个 FS.DLL（等 DLL 做好之后）。

  建议先 1，等 DLL 稳定后再考虑拆出去。slot 号段已在 [02](02-decisions.md) D8 的 resman 0x60 段预留。
- loader 和运行时各有一份 FAT32，要不要共用源码？loader 刻意不依赖 lib，共用的话需要把 FAT 解析写成不依赖 IO 的纯函数（传入"读扇区"回调）。
