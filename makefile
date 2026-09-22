TOOL_ASM	= nasm
TOOL_C		= gcc
TOOL_LD		= ld

ASFLAGS	= -f elf32
CFLAGS   = -m32 -ffreestanding -fno-pie \
		   -fno-stack-protector -fno-builtin \
           -nostdlib -mno-80387 -mno-fp-ret-in-387 \
		   -mno-mmx -mno-sse -mno-sse2 -Wall \
		   -Wextra -Werror -Os -std=gnu11 -Ikernel
LDFLAGS  = -m elf_i386 -T link.ld -nostdlib

MBR		= mbr.bin
STAGE2	= stage2.bin
KERNEL	= kernel.bin
ELF		= stage2.elf
IMG		= lwcnc.img

IMG_SECTORS		= 131040
PART_LBA		= 2048

FSROOT	= fsroot
# i will write a clang fat32 make

OBJS	= boot/stage2.o

DEVICE	= /dev/sda

all: bin

$(MBR): boot/mbr.s
	$(TOOL_ASM) -f bin -DIMG_SECTORS=$(IMG_SECTORS) -DPART_LBA=$(PART_LBA) $< -o $@

boot/stage2.o: boot/stage2.s
	$(TOOL_ASM) $(ASFLAGS) -DPART_LBA=$(PART_LBA) $< -o $@

boot/%.o: boot/%.s
	$(TOOL_ASM) $(ASFLAGS) $< -o $@

$(ELF): $(OBJS) link.ld
	$(TOOL_LD) $(LDFLAGS) -o $@ $(OBJS)

$(STAGE2): $(ELF)
	objcopy -O binary --only-section=.stage2.entry $< $@

$(IMG): $(MBR) $(STAGE2)
	rm -f $@
	truncate -s $$(( $(IMG_SECTORS) * 512)) $@
	dd if=$(MBR)	of=$@ bs=512 count=1 seek=0 conv=notrunc status=none
	dd if=$(STAGE2) of=$@ bs=512 seek=1 conv=notrunc status=none

bin: $(IMG)

resetimg:
	rm -f $(IMG)
	$(MAKE) $(IMG)

run: $(IMG)
	bochs -f bochsrc -q

clean:
	rm -f boot/*.o $(IMG)

.PHONY: all bin resetimg run clean