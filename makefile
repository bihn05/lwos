# 工具定义
NASM := nasm
GCC := gcc
LD := ld
OBJCOPY := objcopy
NM := nm
DD := dd
BOCHS := bochs
BOCHSDBG := bochsdbg

# 编译选项
NASM_FLAGS_BIN := -f bin
NASM_FLAGS_ELF := -f elf32
GCC_FLAGS := -m32 -Wimplicit-function-declaration -w -fno-builtin -ffreestanding \
             -nodefaultlibs -nostdinc -nostdlib -fno-pic -fno-pie \
             -fno-stack-protector -Ikernel/
LD_FLAGS := -m elf_i386 -static -Ttext 0x10000

# 目标文件定义
BIN_TARGETS := mbr.bin fat.bin clsheap.bin loader.bin
KERNEL_OBJS := kernel/start.o \
				kernel/main.o \
				kernel/int.o \
				kernel/segment.o \
				kernel/mm/schedule.o \
				kernel/mm/vm.o
SYSTEM_FILES := system.bin system.map master.img

# 默认目标
all: master.img

# 清理目标
clean:
	@echo "--------------------Clean Stage"
	rm -f master.img *.bin system.map
	rm -f kernel/kernel.bin kernel/*.o
	rm -f kernel/mm/*.o loader/*.o loader/*.bin
	rm -f external/*.elf

# 基础系统构建
base: $(BIN_TARGETS) kernel/kernel.bin system.bin system.map

# 汇编二进制文件
%.bin: %.s
	$(NASM) $(NASM_FLAGS_BIN) $< -o $@

# 内核汇编文件
kernel/%.o: kernel/%.s
	$(NASM) $(NASM_FLAGS_ELF) $< -o $@

kernel/mm/%.o: kernel/mm/%.s
	$(NASM) $(NASM_FLAGS_ELF) $< -o $@

# 内核C文件编译
kernel/main.o: kernel/main.c
	$(GCC) $(GCC_FLAGS) -c $< -o $@

# 链接内核
kernel/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LD_FLAGS) -o $@ $^ -L"kernel/"

# 系统文件生成
system.bin: kernel/kernel.bin
	$(OBJCOPY) -O binary $< $@

system.map: kernel/kernel.bin
	$(NM) $< | sort > $@

external/test.elf: kernel/dev/test.c
	$(GCC) -m32 -nostdlib -fno-builtin -fno-pic -fno-pie -no-pie -static -e main -Ttext 0x2000000 $< -o $@

# 创建磁盘镜像
master.img: base img.img external/test.elf
	@echo "--------------------Generating Image"
	cp img.img master.img
	$(DD) bs=512 count=1 seek=0 if=mbr.bin of=master.img conv=notrunc
	$(DD) bs=512 count=7 seek=1 if=loader.bin of=master.img conv=notrunc
	$(DD) bs=512 count=8192 seek=8 if=fat.bin of=master.img conv=notrunc
	$(DD) bs=512 count=512 seek=8200 if=clsheap.bin of=master.img conv=notrunc
	$(DD) bs=512 count=200 seek=8728 if=system.bin of=master.img conv=notrunc
	$(DD) bs=512 count=8 seek=8928 if=test.txt of=master.img conv=notrunc
	$(DD) bs=512 count=32 seek=8936 if=external/test.elf of=master.img conv=notrunc

# 运行目标
run: master.img
	@echo "--------------------Power On"
	$(BOCHS) -q
	@echo "--------------------Emulation Terminated"

# 调试目标
debug: master.img
	@echo "--------------------Power On (Debug)"
	$(BOCHSDBG) -q

# 仅构建不运行
build: master.img
	@echo "Build completed"

# 伪目标声明
.PHONY: all clean base run debug build

# 技巧必须如此 最好人如机器
# 题材不可那般 没头没脑简单
# 尔为画匠我为官 指挥操纵按机关
# 创作应加严管 百顺千依好办