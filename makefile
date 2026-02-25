# ==========================================
# 操作系统构建系统 (精修版)
# ==========================================

# 1. 目录定义
BUILD_DIR   := build
INCLUDE_DIR := include
LIB_DIR     := lib
ASM_DIR     := asm
KERNEL_DIR  := kernel
SIM_DIR     := sim

# 2. 工具定义
NASM     := nasm
GCC      := gcc
LD       := ld
OBJCOPY  := objcopy
NM       := nm
DD       := dd
BOCHS    := bochs
BOCHSDBG := bochsdbg

# 3. 编译与链接选项
NASM_FLAGS_BIN := -f bin
NASM_FLAGS_ELF := -f elf64
GCC_FLAGS := -m64 -mno-red-zone -mcmodel=large -mno-mmx -mno-sse -mno-sse2 \
             -Wimplicit-function-declaration -w -fno-builtin -ffreestanding \
             -nodefaultlibs -nostdinc -nostdlib -fno-pic -fno-pie \
             -fno-stack-protector -I$(INCLUDE_DIR)
# 显式指定入口点为 _start (确保 start.s 中有 global _start)
LD_FLAGS := -m elf_x86_64 -static -T kernel/kernel.ld -L$(LIB_DIR)

# 4. 目标文件定义
ASM_SRCS    := $(wildcard $(ASM_DIR)/*.s)
# 确保 BIN_TARGETS 映射到 build 目录
BIN_TARGETS := $(patsubst $(ASM_DIR)/%.s, $(BUILD_DIR)/%.bin, $(ASM_SRCS))

KERNEL_C_SRCS := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_S_SRCS := $(shell find $(KERNEL_DIR) -name "*.s")
KERNEL_C_OBJS := $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/kernel/%.o, $(KERNEL_C_SRCS))
KERNEL_S_OBJS := $(patsubst $(KERNEL_DIR)/%.s, $(BUILD_DIR)/kernel/%.o, $(KERNEL_S_SRCS))

ALL_KERNEL_OBJS := $(KERNEL_S_OBJS) $(KERNEL_C_OBJS)

KERNEL_OBJS     := $(BUILD_DIR)/kernel/start.o $(filter-out $(BUILD_DIR)/kernel/start.o, $(ALL_KERNEL_OBJS))

SYSTEM_BIN := $(BUILD_DIR)/system.bin
SYSTEM_MAP := $(BUILD_DIR)/system.map
MASTER_IMG := $(SIM_DIR)/master.img

# ==========================================
# 5. 构建规则
# ==========================================
.PHONY: all clean run debug build

all: build

build: $(MASTER_IMG)

clean:
	@echo "-------------------- 清理 Stage"
	rm -rf $(BUILD_DIR)
	rm -f $(MASTER_IMG)

# 核心修正：BIN_TARGETS 必须作为 MASTER_IMG 的依赖项
$(MASTER_IMG): $(BIN_TARGETS) $(SYSTEM_BIN) $(SYSTEM_MAP)
	@echo "-------------------- 生成 Image"
	@mkdir -p $(SIM_DIR)
	cp $(SIM_DIR)/img.img $(MASTER_IMG)
	$(DD) bs=512 count=1 seek=0 if=$(BUILD_DIR)/mbr.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=7 seek=1 if=$(BUILD_DIR)/loader.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=8192 seek=8 if=$(BUILD_DIR)/fat.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=512 seek=8200 if=$(BUILD_DIR)/clsheap.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=200 seek=8728 if=$(SYSTEM_BIN) of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=8 seek=8928 if=sim/test.txt of=$(MASTER_IMG) conv=notrunc

# 静态模式规则：处理 asm/ 下的二进制文件
$(BIN_TARGETS): $(BUILD_DIR)/%.bin: $(ASM_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS_BIN) $< -o $@

# 处理内核目标文件
$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS_ELF) $< -o $@

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(GCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LD_FLAGS) -o $@ $(KERNEL_OBJS)

$(SYSTEM_BIN): $(BUILD_DIR)/kernel.bin
	$(OBJCOPY) -O binary $< $@

$(SYSTEM_MAP): $(BUILD_DIR)/kernel.bin
	$(NM) $< | sort > $@

run: build
	cd $(SIM_DIR) && $(BOCHS) -q

debug: build
	cd $(SIM_DIR) && $(BOCHSDBG) -q

# 笼鸡有食刀汤近
# 野鹤无粮天地宽