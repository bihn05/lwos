TOOL_ASM	= nasm
TOOL_C		= gcc
TOOL_LD		= ld

ASFLAGS	= -f elf32
CFLAGS   = -m32 -ffreestanding -fno-pie \
		   -fno-stack-protector -fno-builtin \
           -nostdlib -mno-80387 -mno-fp-ret-in-387 \
		   -mno-mmx -mno-sse -mno-sse2 -Wall \
		   -Wextra -Werror -Os -std=gnu11 -Icommon -Idev
LDFLAGS  = -m elf_i386 -T link.ld -nostdlib

BIN_DIR = bin

MBR          = $(BIN_DIR)/mbr.bin
ELF          = $(BIN_DIR)/stage2.elf
STAGE2       = $(BIN_DIR)/stage2.bin
LOADER_ELF   = $(BIN_DIR)/loader.elf
LOADER_BIN   = $(BIN_DIR)/loader.bin
MONITOR_ELF  = $(BIN_DIR)/monitor.elf
MONITOR_BIN  = $(BIN_DIR)/monitor.bin
ABI_ELF      = $(BIN_DIR)/abi.elf
ABI_BIN      = $(BIN_DIR)/abi.bin

KERNEL      = kernel.exe

IMG		= lwcnc.img

IMG_SECTORS		= 131040
PART_LBA		= 2048

FSROOT	= fsroot
MKFAT	= tools/mkfat/mkfat

OBJS	= boot/stage2.o

COMMON_HDRS	= $(wildcard common/*.h)

DEVICE	= /dev/sda

all: bin

$(BIN_DIR):
	mkdir -p $@

$(MKFAT): tools/mkfat/mkfat.c
	gcc $< -o $@ 

$(MBR): boot/mbr.s | $(BIN_DIR)
	$(TOOL_ASM) -f bin -DIMG_SECTORS=$(IMG_SECTORS) -DPART_LBA=$(PART_LBA) $< -o $@

boot/loader.o: boot/loader.c $(COMMON_HDRS)
	$(TOOL_C) $(CFLAGS) -c $< -o $@

$(LOADER_ELF): boot/loader.o boot/loader.ld | $(BIN_DIR)
	$(TOOL_LD) -m elf_i386 -T boot/loader.ld -nostdlib -o $@ boot/loader.o

$(LOADER_BIN): $(LOADER_ELF) | $(BIN_DIR)
	objcopy -O binary $< $@

# dev/: 设备驱动. 有状态的 (text) 只进 ABI.BIN; 无状态的 (io) 谁用谁链
dev/%.o: dev/%.c $(wildcard dev/*.h) $(COMMON_HDRS)
	$(TOOL_C) $(CFLAGS) -c $< -o $@

# ABI.BIN: 固定在 100000H 的服务库
ABI_OBJS = 	abi/abi.o \
			dev/text.o \
			dev/io.o \
			dev/kbd.o

abi/%.o: abi/%.c $(wildcard dev/*.h) $(COMMON_HDRS)
	$(TOOL_C) $(CFLAGS) -c $< -o $@

$(ABI_ELF): $(ABI_OBJS) abi/abi.ld | $(BIN_DIR)
	$(TOOL_LD) -m elf_i386 -T abi/abi.ld -nostdlib \
	--no-warn-rwx-segments \
	-o $@ $(ABI_OBJS)

$(ABI_BIN): $(ABI_ELF)
	objcopy -O binary $< $@

# MONITOR.BIN: 固定在 200000H, 控制台全部走 ABI
MONITOR_OBJS = 	monitor/head.o \
				monitor/monitor.o \
				monitor/isr.o \
				monitor/idt.o \
				dev/io.o

monitor/isr.o: monitor/isr.s
	$(TOOL_ASM) $(ASFLAGS) $< -o $@

monitor/%.o: monitor/%.c $(wildcard monitor/*.h dev/*.h) $(COMMON_HDRS)
	$(TOOL_C) $(CFLAGS) -c $< -o $@

$(MONITOR_ELF): $(MONITOR_OBJS) monitor/monitor.ld | $(BIN_DIR)
	$(TOOL_LD) -m elf_i386 -T monitor/monitor.ld -nostdlib \
	--no-warn-rwx-segments \
	-o $@ $(MONITOR_OBJS)

$(MONITOR_BIN): $(MONITOR_ELF)
	objcopy -O binary $< $@

boot/stage2.o: boot/stage2.s
	$(TOOL_ASM) $(ASFLAGS) -DPART_LBA=$(PART_LBA) $< -o $@

boot/%.o: boot/%.s
	$(TOOL_ASM) $(ASFLAGS) $< -o $@

$(ELF): $(OBJS) link.ld | $(BIN_DIR)
	$(TOOL_LD) $(LDFLAGS) -o $@ $(OBJS)

$(STAGE2): $(ELF)
	objcopy -O binary --only-section=.stage2.entry $< $@

$(FSROOT)/LOADER.BIN: $(LOADER_BIN)
	mkdir -p $(FSROOT)
	cp $< $@

$(FSROOT)/MONITOR.BIN: $(MONITOR_BIN)
	mkdir -p $(FSROOT)
	cp $< $@

$(FSROOT)/ABI.BIN: $(ABI_BIN)
	mkdir -p $(FSROOT)
	cp $< $@

$(FSROOT)/BOOT.INI: boot/boot.ini
	mkdir -p $(FSROOT)
	cp $< $@

$(FSROOT)/TEST1.BIN: test/TEST1.BIN
	mkdir -p $(FSROOT)
	cp $< $@

fsroot: $(FSROOT)/LOADER.BIN \
		$(FSROOT)/BOOT.INI \
		$(FSROOT)/ABI.BIN \
		$(FSROOT)/MONITOR.BIN \
		$(FSROOT)/TEST1.BIN

$(IMG): $(MBR) $(STAGE2) $(MKFAT) fsroot $(LOADER_BIN) $(wildcard $(FSROOT)/*)
	rm -f $@
	truncate -s $$(( $(IMG_SECTORS) * 512)) $@
	dd if=$(MBR)	of=$@ bs=512 count=1 seek=0 conv=notrunc status=none
	dd if=$(STAGE2) of=$@ bs=512 seek=1 conv=notrunc status=none
	$(MKFAT) --image $@ --total-sectors $(IMG_SECTORS) --part-lba $(PART_LBA) \
	    --fsroot $(FSROOT)
all: $(IMG)

resetimg:
	rm -f $(IMG)
	$(MAKE) $(IMG)

run: $(IMG)
	bochs -f bochsrc -q

clean:
	rm -f boot/*.o monitor/*.o dev/*.o abi/*.o $(MKFAT) $(IMG)
	rm -rf $(BIN_DIR) $(FSROOT)

.PHONY: all resetimg run clean fsroot