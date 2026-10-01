<p align=left>
  <a href="https://lwos.dev/">
    <img alt="LWOS" src="https://lwos.dev/assets/img/wordmark.png" width="80%">
  </a>
</p>

# LWOS

**所有的实现都是手工编写的...**

一个面向CNC (计算机数控) 铣削设备的业余操作系统，外观和操作感受受到 IBM-PC 启发。
注意：LWOS在风格上类似IBM-PC，但是不依赖BIOS。固件支持（自定义BIOS、IBM兼容机）属于另一个独立项目，不是本项目范围内的。

> **状态:** LWOS在早期阶段正在积极重构。仓库正在从零开始重建和标准化，因此API、目录结构可能随时变更，不会通知，磁盘结构是FAT32+自制LOADER。

## 目前可用的功能

- **MBR** — 用于引导系统盘和分区表。
- **FAT32 (read-only)** — 简化版FAT32读取实现在loader里，以后会写高级的多级目录+读写。
- **`tools/mkfat`** — 一个用于创建FAT镜像的小工具，用C语言实现。

其他所有内容（shell、实用工具、CNC栈）还在规划中，没全实现。

## 正在规划的

- `MONITOR.BIN` — 仿照MS-DOS `DEBUG.COM`但是包含更多ABI的一个调试器
- `EDIT.EXE` — 仿照MS-DOS `EDIT.COM`的文本编辑器
- 关于设计、内存布局的存档

## 仓库结构
    boot/     MBR + STAGE2（实模式，磁盘保留扇区）
    loader/   LOADER.BIN 与 BOOT.INI
    lib/      库；lib/include 为公开头文件（abi.h 等）
      abi/    ABI.BIN，src/ 与 include/（私有头）分开
    app/      运行在 ABI 之上的程序（monitor、test1），各自 src/ 与 include/ 分开
    build/    中间产物（.o/.d），bin/ 为成品
    fsroot/ 用于构建FAT32的暂用目录
    tools/  宿主机侧实用工具
    report/ 评审笔记（review）
    SPEC/   规格书
    spec2/  v2 规格书
    site/   lwos.dev 站点（Astro，独立于 OS 构建，见 site/README.md）

## 构建与运行

要求：

- 有 `make` 的 Unix 类环境
- C 编译器
- [QEMU](https://www.qemu.org/) or [Bochs](https://bochs.sourceforge.io/)

在模拟器中构建并启动：

```sh
make run
```

## 许可证

本项目以 **GNU General Public License v3.0**（`GPL-3.0-only`，仅第 3 版）发布，完整条款见 [LICENSE](LICENSE)——该文件是 gnu.org 官方文本的逐字节副本，未做任何改动，版权声明因此写在这里。

    Copyright (C) 2026 LWOS-dev

    本程序是自由软件：你可以依据自由软件基金会发布的 GNU 通用公共许可证第 3 版
    重新分发和/或修改它。本程序按"原样"分发，不提供任何担保。

    SPDX-License-Identifier: GPL-3.0-only