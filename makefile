.DEFAULT_GOAL := all

TOOL_ASM	= nasm
TOOL_C		= gcc
TOOL_LD		= ld

# 源码按运行时的四个世界分层, 每个世界自己的构建规则写在各自目录的 build.mk:
#   boot/    MBR + STAGE2, 实模式, 装进磁盘保留扇区
#   loader/  LOADER.BIN, 0x10000, 自带驱动, 不依赖 lib
#   lib/     库. lib/include 是对外公开的头 (abi.h 等), 谁都能用;
#            lib/src 是共享源码, 按 APP / KERN 显式选择并分别编译;
#            lib/abi 是 ABI.BIN (0x100000), 私有头文件只给它自己用
#   app/     跑在 ABI 之上的程序, 只能看见 lib/include
# 中间产物全部放 build/, 成品放 bin/, 源码目录里不留 .o

# ---------------------------------------------------------------- 编译选项分层
# KERN: loader / lib / 以后的 resman、sys. 禁止浮点: 这些代码会在任意 app
#       的上下文里运行, 碰了 FPU 就会弄坏 app 没保存的浮点状态.
#       写了 float 时 gcc 不会报错, 而是改调软浮点 (__adddf3 等);
#       KERN 层不链 libgcc, 所以会在链接时报 undefined reference.
# APP:  app. 允许 x87 (FPU 由 ABI 初始化), 链 libgcc 补 64 位除法和
#       long long <-> double 之类的转换.
ASFLAGS		= -f elf32
CFLAGS_BASE	= -m32 -ffreestanding -fno-pie \
			  -fno-stack-protector -fno-builtin -nostdlib \
			  -mno-mmx -mno-sse -mno-sse2 \
			  -Wall -Wextra -Os -std=gnu11 -MMD -MP
CFLAGS_KERN	= $(CFLAGS_BASE) -mno-80387 -mno-fp-ret-in-387
CFLAGS_APP	= $(CFLAGS_BASE) -mfpmath=387
LDFLAGS		= -m elf_i386 -nostdlib --no-warn-rwx-segments
LIBGCC		:= $(shell $(TOOL_C) -m32 -print-libgcc-file-name)

BIN_DIR		= bin
BUILD_DIR	= build

LIB_INC		= -Ilib/include

IMG			= lwcnc.img
IMG_SECTORS	= 131040
PART_LBA	= 2048

FSROOT		= fsroot
MKFAT		= tools/mkfat/mkfat

DEVICE		= /dev/sda

# 各世界往这里追加自己要放进 FAT 分区的文件, 再声明它从哪来
FSROOT_FILES =

all: $(IMG)

# ---------------------------------------------------------------- 各世界
# 必须在镜像规则之前 include: 依赖列表里的 $(FSROOT_FILES) 是立即展开的
include boot/build.mk
include loader/build.mk
include lib/build.mk
include lib/abi/build.mk
include app/monitor/build.mk
include app/test1/build.mk

# ---------------------------------------------------------------- 通用
$(BIN_DIR)/%.bin: $(BIN_DIR)/%.elf
	objcopy -O binary $< $@

$(MKFAT): tools/mkfat/mkfat.c
	gcc $< -o $@

# ---------------------------------------------------------------- fsroot / 镜像
$(FSROOT_FILES):
	@mkdir -p $(@D)
	cp $< $@

fsroot: $(FSROOT_FILES)

$(IMG): $(MBR) $(STAGE2) $(MKFAT) $(FSROOT_FILES)
	rm -f $@
	truncate -s $$(( $(IMG_SECTORS) * 512)) $@
	dd if=$(MBR)	of=$@ bs=512 count=1 seek=0 conv=notrunc status=none
	dd if=$(STAGE2) of=$@ bs=512 seek=1 conv=notrunc status=none
	$(MKFAT) --image $@ --total-sectors $(IMG_SECTORS) --part-lba $(PART_LBA) \
	    --fsroot $(FSROOT)

resetimg:
	rm -f $(IMG)
	$(MAKE) $(IMG)

run: $(IMG)
	bochs -f bochsrc -q

# ---------------------------------------------------------------- 在线试玩
# 镜像不进仓库: CI 每次 push 到 main 会 make webimg 然后把产物覆盖挂到
# 固定 tag 的 GitHub Release 上 (滚动更新), demo 页从那里匿名拉取.
# 这个目标只用于本地调试 site/ 的 demo 页, 输出目录在 .gitignore 里.
DEMO_IMG_GZ	= site/public/assets/demo/lwcnc.img.gz

$(DEMO_IMG_GZ): $(IMG)
	@mkdir -p $(@D)
	gzip -9 -c $(IMG) > $@

webimg: $(DEMO_IMG_GZ)

clean:
	rm -f $(MKFAT) $(IMG) $(DEMO_IMG_GZ)
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(FSROOT)

.PHONY: all resetimg run clean fsroot webimg

-include $(shell find $(BUILD_DIR) -name '*.d' 2>/dev/null)
