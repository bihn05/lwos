# 20 — PCI 枚举

## 目标

扫一遍 PCI 总线，得到一张**设备表**：每个功能的 BDF、ID、class、BAR（含大小）、IRQ。
驱动靠这张表找到自己的设备，不再各扫各的。

## 现状

- ABI 已导出配置空间原语：`lw_pci_present` / `lw_pci_read` / `lw_pci_write`（slot 0x40–0x42）。
- 寄存器偏移常量在 `lib/abi/include/pci.h`，**只有 ABI 自己看得见**。
- 还没有任何枚举代码。

## 依赖

- 前置：ABI PCI slot（已完成）。
- 后继：[21 AHCI](21-ahci.md)、[30 网卡](30-net.md)、[60 GUI](60-gui.md)（显卡 BAR，可选）。

## 设计

### 文件布局

```
lib/include/pci_def.h     寄存器偏移、class 码、命令寄存器位。纯常量，谁都能 include
lib/abi/include/pci.h     改成 #include "pci_def.h"，自己只留函数原型
lib/pcienum/              枚举代码（静态库，KERN 安全，不用浮点）
  include/pcienum.h
  src/pcienum.c
```

枚举代码做成**静态库**，因为它先后有两个使用者：

1. **现在**：monitor 链接它，做 `lspci` 类命令，resman 还不存在也能开工。
2. **以后**：resman 链接它，作为设备表的唯一拥有者（[02](02-decisions.md) D3）。
   那时 monitor 改成向 resman 查询。

### 数据结构

```c
typedef struct {
    BYTE  bus, dev, func, hdr_type;
    WORD  vendor, device;
    BYTE  class_code, subclass, prog_if, revision;
    BYTE  irq_line, irq_pin;
    DWORD bar[6];        /* 已去掉类型位的基址；IO BAR 为端口号 */
    DWORD bar_size[6];   /* 0 = 未实现 */
    BYTE  bar_flags[6];  /* PCI_BAR_IO / PCI_BAR_MEM64 / PCI_BAR_PREFETCH */
} PCI_DEV;

int      pci_enum(PCI_DEV *tab, int max);   /* 返回找到的个数 */
PCI_DEV *pci_find_id(PCI_DEV *tab, int n, WORD ven, WORD dev);
PCI_DEV *pci_find_class(PCI_DEV *tab, int n, BYTE cls, BYTE sub, BYTE progif); /* progif=0xFF 表示不限 */
const char *pci_class_name(BYTE cls, BYTE sub);
```

### 扫描规则

- 暴力扫 bus 0–255、dev 0–31。vendor 读到 `0xFFFF` 表示不存在。
- func 0 的 header type 第 7 位（`PCI_HDR_MULTIFN`）为 1 时，才继续扫 func 1–7。
- 只处理 header type 0（普通设备）；type 1（桥）记下来但不解析 BAR。

### BAR 测大小

1. **先关掉命令寄存器的 IO/内存译码位**（COMMAND 的 bit0、bit1），否则写全 1 的瞬间设备会去响应一段错误的地址。
2. 保存原值 → 写 `0xFFFFFFFF` → 读回 → 恢复原值 → 恢复命令寄存器。
3. 大小 = `~(读回值 & 掩码) + 1`。内存 BAR 掩码 `~0xF`，IO BAR 掩码 `~0x3`。
4. 类型位 `[2:1] = 10b` 的是 64 位 BAR，占用下一个 BAR 槽。高 32 位不为 0 时，32 位系统访问不了，要标记出来。

### 驱动要记得的一件事

使用 DMA 的设备（AHCI、网卡）必须把命令寄存器的 **Bus Master 位（bit2）** 置 1，
否则描述符写好了设备也读不到内存。这件事由驱动的 probe 自己做，不归枚举管。

## 里程碑

| # | 内容 | demo / 验收 |
|---|---|---|
| E1 | `pci_def.h` 抽出来；`pci_enum` + `pci_class_name` | monitor 加 `pl` 命令，每行一个功能：`bb:dd.f vvvv:dddd cc.ss.pp 名字`。Bochs i440fx 下应看到主桥 8086:1237、PIIX3 8086:7000、IDE 8086:7010、VGA 1234:1111 |
| E2 | BAR 解码和测大小 | `pd bb:dd.f` 打印 6 个 BAR：类型、基址、大小。VGA 的 BAR0 应等于 `lw_get_fb()` |
| E3 | `pci_find_id` / `pci_find_class` | QEMU `-machine q35` 下 `pci_find_class(1,6,1)` 找到 ICH9 AHCI 8086:2922 |

## 待决

- 设备表最多放多少项？建议先写死 64。
- 要不要走 capability 链表（MSI、电源管理）？现阶段不需要，留到中断阶段再看。
