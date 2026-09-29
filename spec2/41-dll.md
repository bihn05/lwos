# 41 — DLL

## 目标

多个程序共享一份代码（第一个候选是图形库 GFX.DLL），可以单独更新，不用重新链接调用方。

## 现状

无。LWP 头里已预留 `flags.bit0 = DLL` 和导出表字段（见 [40](40-lwp-exe.md)）。

## 依赖

- 前置：[40 LWP 加载器](40-lwp-exe.md) 的 X3（重定位正确）。
- 后继：[60 GUI](60-gui.md)（GFX.DLL）、以后把驱动/FS 拆出去。

## 设计：沿用 ABI 的"表"模式 〔荐〕

有两种做法：

| | 表 DLL（推荐先做） | 隐式导入 |
|---|---|---|
| 调用方写法 | `t = rm_dll_load("GFX.DLL")`，然后用宏查表调用 | 直接写 `gfx_fill_rect(...)` |
| 加载器要做的 | 加载 + 重定位 + 返回表地址 | 还要解析导入表、按名字查找、回填 IAT |
| mkexe 要做的 | 什么都不用加 | 要生成导入表 |
| 和现有设计 | **和 ABI 完全一样**，你已经熟悉 | 新机制 |

**先做表 DLL**。你的 ABI 本来就是一个"表 DLL"，只不过地址固定。DLL 就是**地址不固定、由 resman 加载的 ABI**。

### 规则

- DLL 映像开头是一张 slot 表，**和 ABI 同构**：
  - slot 0x00 magic（每个 DLL 自己定，例如 'GFX\0'）
  - slot 0x01 初始化函数
  - slot 0x02–0x0F 保留
  - 0x10 起是导出函数
- 同样的铁律：**只追加，不改号，不复用，不删除**。
- DLL 提供一个公开头文件（例如 `lib/include/gfx.h`），写法和 `abi.h` 一样：一组 `LW_CALL` 风格的宏。
- slot 表里的函数指针在链接时是 RVA，会被重定位表覆盖到，加载后自动变成真实地址。**不需要额外处理**。

### resman 接口

```c
PVOID *rm_dll_load(const char *name);   /* 已加载就引用计数 +1，返回同一张表 */
void   rm_dll_free(PVOID *table);       /* 计数归零才卸载 */
```

### 初始化

DLL 第一次加载时调用 slot 0x01，传入 `LW_ENV`（ABI 和 resman 的表地址），DLL 自己存下来。
这和 EXE 的 `crt0` 是同一套代码。

### 注意：DLL 的全局变量是全系统共享的

平坦、无分页，所以 DLL 的 `.data`/`.bss` 只有一份，所有调用它的任务都看到同一份。

- 需要按调用方区分的状态，要做成**句柄/上下文结构**，由调用方持有。例如 GFX 的"当前画布"应该是 `gfx_surface_t *` 参数，而不是 DLL 里的全局变量。
- 任务阶段之后，DLL 里的共享状态同样要按 [02](02-decisions.md) D5 加锁。

## 里程碑

| # | 内容 | demo / 验收 |
|---|---|---|
| L1 | `rm_dll_load` / `rm_dll_free`，引用计数 | 一个只导出 `add(a,b)` 的 TEST.DLL；monitor 加载它、调用、打印结果 |
| L2 | EXE 调 DLL | HELLO.LWP 在里面加载 TEST.DLL 并调用 |
| L3 | GFX.DLL | 把 [60](60-gui.md) 的 `lib/gfx` 编成 DLL，monitor 通过它画图 |
| L4 | 更新 DLL 不重编调用方 | 给 GFX.DLL 追加一个函数后重新部署；旧的 HELLO.LWP 仍能运行 |

## 待决

- DLL 依赖另一个 DLL（在初始化函数里调 `rm_dll_load`）：第一版允许，但不检测循环依赖。
- 隐式导入以后再做吗？如果表 DLL 用着顺手，可能永远不需要。
