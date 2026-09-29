TOOL_ASM	= nasm
TOOL_C		= gcc
TOOL_LD		= ld

# 源码按运行时的四个世界分层:
#   boot/    MBR + STAGE2, 实模式, 装进磁盘保留扇区
#   loader/  LOADER.BIN, 0x10000, 自带驱动, 不依赖 lib
#   lib/     库. lib/include 是对外公开的头 (abi.h 等), 谁都能用;
#            lib/abi 是 ABI.BIN (0x100000), 头文件只给它自己用
#   app/     跑在 ABI 之上的程序, 只能看见 lib/include
# 中间产物全部放 build/, 成品放 bin/, 源码目录里不留 .o

ASFLAGS	= -f elf32
CFLAGS   = -m32 -ffreestanding -fno-pie \
		   -fno-stack-protector -fno-builtin \
           -nostdlib -mno-80387 -mno-fp-ret-in-387 \
		   -mno-mmx -mno-sse -mno-sse2 -Wall \
		   -Wextra -Werror -Os -std=gnu11 -MMD -MP
LDFLAGS  = -m elf_i386 -nostdlib --no-warn-rwx-segments

BIN_DIR   = bin
BUILD_DIR = build

LIB_INC   = -Ilib/include

MBR          = $(BIN_DIR)/mbr.bin
STAGE2_ELF   = $(BIN_DIR)/stage2.elf
STAGE2       = $(BIN_DIR)/stage2.bin
LOADER_ELF   = $(BIN_DIR)/loader.elf
LOADER_BIN   = $(BIN_DIR)/loader.bin
ABI_ELF      = $(BIN_DIR)/abi.elf
ABI_BIN      = $(BIN_DIR)/abi.bin
MONITOR_ELF  = $(BIN_DIR)/monitor.elf
MONITOR_BIN  = $(BIN_DIR)/monitor.bin

IMG		= lwcnc.img

IMG_SECTORS		= 131040
PART_LBA		= 2048

FSROOT	= fsroot
MKFAT	= tools/mkfat/mkfat

DEVICE	= /dev/sda

all: $(IMG)

$(MKFAT): tools/mkfat/mkfat.c
	gcc $< -o $@

# ---------------------------------------------------------------- boot/
$(MBR): boot/mbr.s
	@mkdir -p $(@D)
	$(TOOL_ASM) -f bin -DIMG_SECTORS=$(IMG_SECTORS) -DPART_LBA=$(PART_LBA) $< -o $@

$(BUILD_DIR)/boot/stage2.o: boot/stage2.s
	@mkdir -p $(@D)
	$(TOOL_ASM) $(ASFLAGS) -DPART_LBA=$(PART_LBA) $< -o $@

$(STAGE2_ELF): $(BUILD_DIR)/boot/stage2.o boot/stage2.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T boot/stage2.ld -o $@ $<

$(STAGE2): $(STAGE2_ELF)
	objcopy -O binary --only-section=.stage2.entry $< $@

# ---------------------------------------------------------------- loader/
LOADER_OBJS = $(BUILD_DIR)/loader/loader.o

$(BUILD_DIR)/loader/%.o: loader/%.c
	@mkdir -p $(@D)
	$(TOOL_C) $(CFLAGS) $(LIB_INC) -c $< -o $@

$(LOADER_ELF): $(LOADER_OBJS) loader/loader.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T loader/loader.ld -o $@ $(LOADER_OBJS)

# ---------------------------------------------------------------- lib/abi
# ABI.BIN: 固定在 100000H 的服务库, 相当于 hal.dll
ABI_DIR  = lib/abi
ABI_SRCS = abi.c text.c io.c kbd.c ata.c binfo.c tramp.c
ABI_OBJS = $(ABI_SRCS:%.c=$(BUILD_DIR)/$(ABI_DIR)/%.o)

$(BUILD_DIR)/$(ABI_DIR)/%.o: $(ABI_DIR)/src/%.c
	@mkdir -p $(@D)
	$(TOOL_C) $(CFLAGS) $(LIB_INC) -I$(ABI_DIR)/include -c $< -o $@

$(ABI_ELF): $(ABI_OBJS) $(ABI_DIR)/abi.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T $(ABI_DIR)/abi.ld -o $@ $(ABI_OBJS)

# ---------------------------------------------------------------- app/monitor
# MONITOR.BIN: 固定在 200000H, 控制台全部走 ABI
MON_DIR  = app/monitor
MON_OBJS = $(BUILD_DIR)/$(MON_DIR)/head.o \
		   $(BUILD_DIR)/$(MON_DIR)/monitor.o \
		   $(BUILD_DIR)/$(MON_DIR)/isr.o \
		   $(BUILD_DIR)/$(MON_DIR)/idt.o

$(BUILD_DIR)/$(MON_DIR)/%.o: $(MON_DIR)/src/%.c
	@mkdir -p $(@D)
	$(TOOL_C) $(CFLAGS) $(LIB_INC) -I$(MON_DIR)/include -c $< -o $@

$(BUILD_DIR)/$(MON_DIR)/%.o: $(MON_DIR)/src/%.s
	@mkdir -p $(@D)
	$(TOOL_ASM) $(ASFLAGS) $< -o $@

$(MONITOR_ELF): $(MON_OBJS) $(MON_DIR)/monitor.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T $(MON_DIR)/monitor.ld -o $@ $(MON_OBJS)

# ---------------------------------------------------------------- 通用
$(BIN_DIR)/%.bin: $(BIN_DIR)/%.elf
	objcopy -O binary $< $@

# ---------------------------------------------------------------- fsroot / 镜像
FSROOT_FILES = 	$(FSROOT)/LOADER.BIN \
				$(FSROOT)/BOOT.INI \
				$(FSROOT)/ABI.BIN \
				$(FSROOT)/MONITOR.BIN \
				$(FSROOT)/TEST1.BIN

$(FSROOT)/LOADER.BIN:  $(LOADER_BIN)
$(FSROOT)/ABI.BIN:     $(ABI_BIN)
$(FSROOT)/MONITOR.BIN: $(MONITOR_BIN)
$(FSROOT)/BOOT.INI:    loader/boot.ini
$(FSROOT)/TEST1.BIN:   app/test1/TEST1.BIN

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

clean:
	rm -f $(MKFAT) $(IMG)
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(FSROOT)

.PHONY: all resetimg run clean fsroot

-include $(shell find $(BUILD_DIR) -name '*.d' 2>/dev/null)
