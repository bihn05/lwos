# directory define
BUILD_DIR    := build
INCLUDE_DIR  := include
LIB_DIR      := lib
ASM_DIR      := asm
KERNEL_DIR   := kernel
SIM_DIR      := sim

# tools define
NASM     := nasm
GCC      := gcc
LD       := ld
OBJCOPY  := objcopy
NM       := nm
DD       := dd
BOCHS    := bochs
BOCHSDBG := bochsdbg

# compile and linking setting
NASM_FLAGS_BIN := -f bin
NASM_FLAGS_ELF := -f elf32

GCC_FLAGS := -m32 -Wimplicit-function-declaration -w -fno_builtin -ffreestanding \
             -nodefaultlibs -nostdinc -nostdlib -fno-pic -fno-pie \
			 -fno-stack-protector -I$(INCLUDE_DIR)
LD_FLAGS  := -m elf_i386 -static -Ttext 0x10000 -L$(LIB_DIR)

# target file define
# scan asm file automatically, mapping to build directory
BIN_SRCS    := $(wildcard $(ASM_DIR)/*.s)
BIN_TARGETS := $(patsubst $(ASM_DIR)/%.s, %(BUILD_DIR)/%.bin, $(BIN_SRCS))

# scan c source and assembly files automatically, reserve dir sturct mapping to build directory
KERNEL_C_SRCS := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_S_SRCS := $(shell fine $(KERNEL_DIR) -name "*.s")
KERNEL_C_OBJS := $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/kernel/%.o, $(KERNEL_C_SRCS))
KERNEL_S_OBJS := $(patsubst $(KERNEL_DIR)/%.s, $(BUILD_DIR)/kernel/%.o, $(KERNEL_S_SRCS))
KERNEL_OBJS   := $(KERNEL_S_OBJS) $(KERNEL_C_OBJS)

SYSTEM_BIN := $(BUILD_DIR)/system.bin
SYSTEM_MAP := $(BUILD_DIR)/system.map
MASTER_IMG := $(SIM_DIR)/master.img

.PHONY: all clean run debug build

all: build

build: $(MASTER_IMG)
	@echo "============================== build completed"

clean:
	@echo "-------------------- 清理 Stage"
	rm -rf $(BUILD_DIR)
	rm -f $(MASTER_IMG)

$(BUILD_DIR)/%.bin: $(ASM_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS_BIN) $< -o $@

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASM_FLAGS_ELF) $< -o $@

$(BUILD_DIR)/kernel/%.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	$(GCC) $(GCC_FLAGS) -c $< -o $@

$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LD_FLAGS) -o $@ $^

$(SYSTEM_BIN): $(BUILD_DIR)/kernel.bin
	$(OBJCOPY) -O binary $< $@

$(SYSTEM_MAP): $(BUILD_DIR)/kernel.bin
	$(NM) $< | sort > $@

$(MASTER_IMG): $(BIN_TARGETS) $(SYSTEM_BIN) $(SYSTEM_MAP)
	@echo "-------------------- 生成 Image"
	@mkdir -p $(SIM_DIR)
	# 假设 sim 目录下存在一个空白或带分区的底包 img.img
	cp $(SIM_DIR)/img.img $(MASTER_IMG)
	$(DD) bs=512 count=1 seek=0 if=$(BUILD_DIR)/mbr.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=7 seek=1 if=$(BUILD_DIR)/loader.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=8192 seek=8 if=$(BUILD_DIR)/fat.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=512 seek=8200 if=$(BUILD_DIR)/clsheap.bin of=$(MASTER_IMG) conv=notrunc
	$(DD) bs=512 count=200 seek=8728 if=$(SYSTEM_BIN) of=$(MASTER_IMG) conv=notrunc

run: build
	@echo "-------------------- Power On"
	cd $(SIM_DIR) && $(BOCHS) -q
	@echo "-------------------- Emulation Terminated"

debug: build
	@echo "-------------------- Power On (Debug)"
	cd $(SIM_DIR) && $(BOCHSDBG) -q