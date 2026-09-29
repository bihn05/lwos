# boot/: MBR + STAGE2, 纯汇编, 没有 C

MBR			= $(BIN_DIR)/mbr.bin
STAGE2_ELF	= $(BIN_DIR)/stage2.elf
STAGE2		= $(BIN_DIR)/stage2.bin

$(MBR): boot/mbr.s
	@mkdir -p $(@D)
	$(TOOL_ASM) -f bin -DIMG_SECTORS=$(IMG_SECTORS) -DPART_LBA=$(PART_LBA) $< -o $@

$(BUILD_DIR)/boot/stage2.o: boot/stage2.s
	@mkdir -p $(@D)
	$(TOOL_ASM) $(ASFLAGS) -DPART_LBA=$(PART_LBA) $< -o $@

$(STAGE2_ELF): $(BUILD_DIR)/boot/stage2.o boot/stage2.ld
	@mkdir -p $(@D)
	$(TOOL_LD) $(LDFLAGS) -T boot/stage2.ld -o $@ $<

# 只要 .stage2.entry, 覆盖通用的 elf->bin 规则
$(STAGE2): $(STAGE2_ELF)
	objcopy -O binary --only-section=.stage2.entry $< $@
