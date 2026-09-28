# 验收报告 · 第20章 · 构建系统（makefile + 链接脚本）

**对照规范:** SPEC/20-build-system.md（描述的是 v1 的构建系统，只按整体思路对照，不逐项要求一致）
**审查文件:** makefile（当前 127 行）；link.ld（20 行）；boot/loader.ld（20 行）；monitor/monitor.ld（34 行）
**上次报告:** SPEC/20_makefile_01.txt（ACCEPTED，只列了 3 处不影响验收的清理项）
**结论:** REVISE（1 处问题会导致增量构建悄悄产出旧代码，另有 1 处小问题）

## 与规范的思路对照

SPEC/20 里 v1 的做法是"一个 ELF 用 objcopy 拆成两个二进制"。v2 改成了三个独立链接单元（stage2 / loader / monitor，各有自己的 `.ld`），再由 mkfat 把 loader、monitor、BOOT.INI 打包进 FAT32 分区。规范文末"Open questions"里第一个问题就是要不要这样拆，v2 的做法正好回答了这个问题，属于预期内的设计分歧，不算缺陷。以下几条规范思路在 v2 里都还在：

- `IMG_SECTORS` / `PART_LBA` 在 makefile 里只定义一次，再用 `-D` 和命令行参数传给 mbr.s、stage2.s、mkfat（`makefile:27-28,46,82,112`）。
- 链接脚本里的 `ASSERT` 仍然守着关键不变量：stage2 的扇区预算和 0x7C00 上限（`link.ld:14-17`），ABI 表必须在 0x100000、且不能为空（`monitor/monitor.ld:29-30`）。
- 入口函数用 `.text.start` 固定排在最前面，`objcopy -O binary` 之后二进制的第一个字节就是入口。
- 规范里提到的"镜像被原地写过后，mtime 比所有依赖都新，make 就不再重建"这个坑，在 v2 里已经不存在了：`fsroot` 是 phony 目标，镜像每次都会 `rm -f` 后重建（见备注）。

## 验证方法

- 把 boot/、monitor/、tools/、makefile、link.ld 复制到临时目录，在那里构建，不碰工作区里的产物。
- 从干净状态跑 `make all`，再跑 `make -j8 all`，两次都成功退出（exit 0），mkfat 报告打包了 3 个文件。
- 构建完成后立刻再跑一次 `make all` 和 `make -q all`，检查增量行为。
- `touch` 几个 monitor 头文件后再跑 `make all`，检查头文件依赖。
- 往 `fsroot/` 里手动放一个多余文件后再跑 `make all`，检查它会不会被打进镜像。
- 用 `nm` 和 `readelf -l` 核对三个 ELF 的入口地址、`__abi_*`/`__bss_*`/`__stack_top` 等符号，以及装载段。

## 检查结果

### 1. 【缺陷】monitor/boot 的 C 目标没有头文件依赖，改头文件不会触发重编

`makefile:70-71` 的模式规则只依赖 `.c` 文件：
```makefile
monitor/%.o: monitor/%.c
	$(TOOL_C) $(CFLAGS) -c $< -o $@
```
而 v2 的 monitor 已经有 5 个头文件（abi.h / ctx.h / io.h / text.h / stdint.h），被 6 个 .c 互相包含。实测：
```
$ touch monitor/text.h monitor/abi.h monitor/stdint.h
$ make all 2>&1 | grep -E 'gcc|ld '
（无输出——没有任何 .o 被重编，monitor.elf 也没有重新链接）
```
后果：比如修改 `abi.h` 里 `LW_SLOT_*` 的枚举值，`abi.o` 不会重编，ABI 表还是旧的槽位布局，但构建会正常"成功"。这种问题很难发现，只有 `make clean` 之后才会消失。SPEC/20 在 Messiness 一节已经点过这个问题（v1 是手写头文件列表，v2 连列表都没有），当时文件少，还只是"脆弱"；现在头文件多了，已经变成实际的错误构建风险。

建议（由你自己改）：在 `CFLAGS` 里加 `-MMD -MP`，makefile 末尾加 `-include $(wildcard boot/*.d monitor/*.d)`，`clean` 里顺带删掉 `*.d`。这也是规范 Open questions 里提出的方案。**判定：需修复。**

### 2. 【小问题】`fsroot/` 里的残留文件会被一起打进镜像

`$(IMG)` 规则调用 `mkfat --fsroot fsroot`，mkfat 会把目录里所有普通文件都打包进去。但 `fsroot/` 只在 `make clean` 时才会被清空。实测往 `fsroot/` 里放一个 `JUNK.TXT` 后，mkfat 的输出从 `3 file(s)` 变成了 `4 file(s)`。以后改名或删掉某个 FS 文件（比如 `MONITOR.BIN` 改名成 `KERNEL.BIN`）时，旧文件会一直留在镜像里，直到下次 clean。现在只有 3 个固定文件，还不会出错，所以归为小问题。可以在 `$(IMG)` 规则里先 `rm -rf $(FSROOT)` 再重新拷贝，或者干脆不用 `--fsroot`，改成显式传 `--extra-file NAME=PATH`。**判定：建议修复，不阻塞。**

### 3. 链接产物核对正确

- `stage2.bin` 为 1590 字节，在 `STUB_SECTS=8`（4096 字节）的预算内；`link.ld` 的两个 ASSERT 都没有触发。
- `loader.elf`：`loader_main` = 0x10000，和 `boot/stage2.s:9` 的 `LOADER_SEG equ 0x1000` 以及 `stage2.s:714` 的 `jmp 0x10000` 一致；`__bss_start/__bss_end` = 0x10F90/0x11240，loader.c:568 会自己清零这段。
- `monitor.elf`：`__abi_start` = 0x100000，`monitor_main` = 0x100060（`.abi` 之后紧跟 `.text.start`）；`__stack_top` = `__bss_end` = 0x104F60，16KB 栈放在 BSS 末尾。这和 BOOT.INI 的 `[L00100000MONITOR BIN]` / `[P00100000]`，以及 loader 里 `P` 指令读取槽 0~4 的逻辑都对得上。
- 本轮新增的 `MONITOR_OBJS`（6 个目标文件）和 `monitor/isr.o` 的 nasm 规则都正确；上一版 `$(MONITOR_ELF)` 依赖里重复写的 `monitor/monitor.o` 已经删掉。`make -j8` 从干净状态并行构建没有出现竞争问题。

## 备注（非缺陷，仅记录）

- **镜像每次都会完整重建。** `fsroot` 是 `.PHONY`，又是 `$(IMG)` 的依赖，所以 `make -q all` 永远返回 1，每次 `make` 都会重新 truncate + dd + mkfat（约 64MB）。好处是规范里说的 mtime 陷阱不存在了，代价是 `resetimg` 目标实际上已经多余，`$(IMG)` 依赖里的 `$(wildcard $(FSROOT)/*)` 也不起作用（而且 wildcard 在解析 makefile 时就求值了，第一次构建时它本来就是空的）。如果这是有意为之，可以删掉这两处；如果不是，就需要把 `fsroot` 改成真实文件依赖。
- **loader.ld 没有体积上限 ASSERT。** loader 放在 0x10000，stage2 按 cluster 链往这里拷贝，没有长度检查（11_stage2_02 已经记过）。现在 loader 只有 4KB，离 0x9FC00（EBDA）很远，但 stage2 有 ASSERT，monitor 也有，loader 是三个链接单元里唯一什么都不查的。可以补一条类似 `ASSERT(__bss_end <= 0x80000, ...)` 的检查。
- `LW_ABI_MAGIC` 在 `boot/loader.c:447` 又抄写了一遍，没有和 `monitor/abi.h` 共用。这属于第 21 章 ABI 的范畴（SPEC/21_abi_shared_header_01.txt 已经涉及），这里只记一下，因为它和第 1 条的头文件依赖问题会叠加：就算以后改成共享头文件，没有 `-MMD` 的话 loader.o 也不会跟着重编。
- 上次报告的 3 个清理项本轮都没有动，照旧记录，不重复展开：`CFLAGS` 里过时的 `-Ikernel`（`makefile:10`）；没有被任何目标用到的 `KERNEL`/`DEVICE` 变量（`makefile:23,35`）；loader 链接时没加 `--no-warn-rwx-segments`，所以仍然会出现 RWX 警告（`makefile:52`）。
- `all: bin`（`makefile:37`）里的 `bin` 实际匹配的是 `$(BIN_DIR)` 这个目录目标，不是 phony 名字。真正起作用的是 `makefile:114` 的 `all: $(IMG)`，所以无害，只是 37 行没有实际意义。
- `link.ld:3` 的 `STACK_SIZE` 仍然是死变量（11_stage2_02 已记）。`monitor/monitor.ld` 里的 `STACK_SIZE` 是真正在用的。

## 判定

整体思路和 SPEC/20 一致，并且按规范 Open questions 的方向拆成了独立的链接单元，干净构建和并行构建都通过。但第 1 条（没有头文件依赖）在 monitor 已有 5 个共享头文件的情况下会造成"构建成功但产物是旧的"，需要修复后复审。第 2 条和备注项不阻塞。
