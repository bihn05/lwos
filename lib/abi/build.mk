# lib/abi: ABI.BIN, 固定在 100000H 的服务库, 相当于 hal.dll. KERN 层
# 源文件逐个列出: 不在表里的 (比如 pci.c) 不参与链接

ABI_DIR		= lib/abi
ABI_ELF		= $(BIN_DIR)/abi.elf
ABI_BIN		= $(BIN_DIR)/abi.bin
ABI_SRCS	= abi.c \
			  text.c \
			  io.c \
			  kbd.c \
			  ata.c \
			  binfo.c \
			  tramp.c \
			  fpu.c \
			  pci.c 
ABI_OBJS	= $(ABI_SRCS:%.c=$(BUILD_DIR)/$(ABI_DIR)/%.o)

$(BUILD_DIR)/$(ABI_DIR)/%.o: $(ABI_DIR)/src/%.c
	@mkdir -p $(@D)
	$(TOOL_C) $(CFLAGS_KERN) $(LIB_INC) -I$(ABI_DIR)/include -c $< -o $@

$(ABI_ELF): $(ABI_OBJS) $(ABI_DIR)/abi.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T $(ABI_DIR)/abi.ld -o $@ $(ABI_OBJS)

FSROOT_FILES += $(FSROOT)/ABI.BIN
$(FSROOT)/ABI.BIN: $(ABI_BIN)
