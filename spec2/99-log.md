# 99 — 开发日志

新的写在最上面。格式见 [90-roadmap.md](90-roadmap.md)。

---

## 2026-09-30  spec2 建立

- 做到：spec2 初稿（00–90）。ABI 已有 pitch slot（0x34）和 PCI slot（0x40–0x42）
- 下一步：P1 —— GUI G0（`lib/gfx` 的画布 + fill_rect）或 PCI E1（`pci_def.h` + `pl` 命令），哪个有灵感先做哪个
- 坑：画曲线 demo 的 `acc[sy]` 没检查下标，越界写把栈踩坏，Bochs 日志里的表现是一串 GDT index 越界。以后看到这种日志，先找最近写过的数组
