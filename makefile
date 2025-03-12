ENTRYPOINT:=0x70000

CFLAGS:= -m32
CFLAGS+= -fno-builtin
CFLAGS+= -nostdinc
CFLAGS+= -fno-pic
CFLAGS+= -fno-pie
CFLAGS+= -nostdlib
CFLAGS+= -fno-stack-protector
CFLAGS:=$(strip ${CFLAGS})

INCLUDE:=-I./KERNEL

%.BIN: %.S
	nasm -f bin $< -o $@

./KERNEL/%.o: ./KERNEL/%.S
	nasm -f elf32 $< -o $@

./KERNEL/%.o: ./KERNEL/%.c
	gcc $(CFLAGS) $(INCLUDE) -c $< -o $@

./KERNEL/kernel.bin: ./KERNEL/START.o ./KERNEL/main.o
	ld -m elf_i386 -static $^ -o $@ -Ttext $(ENTRYPOINT)

system.bin: ./KERNEL/kernel.bin
	objcopy -O binary $< $@

system.map: ./KERNEL/kernel.bin
	nm $< | sort > $@

master.img: MBR.BIN FAT.BIN FDT.BIN WHEEL.BIN BASFNT.FNT system.bin system.map
	cp img.img master.img
	dd if=MBR.BIN of=master.img bs=512 seek=0 count=1 conv=notrunc
	dd if=FAT.BIN of=master.img bs=512 seek=1 count=128 conv=notrunc
	dd if=FDT.BIN of=master.img bs=512 seek=129 count=128 conv=notrunc
	dd if=WHEEL.BIN of=master.img bs=512 seek=257 count=1 conv=notrunc
	dd if=BASFNT.FNT of=master.img bs=512 seek=258 count=8 conv=notrunc
	# dd if=LWLDR.BIN of=master.img bs=512 seek=266 count=4 conv=notrunc
	dd if=system.bin of=master.img bs=512 seek=270 count=64 conv=notrunc

test: ./master.img

.PHONY: clean
clean:
	rm -rf *.BIN
	rm -rf ./KERNEL/*.o
	rm -rf ./KERNEL/kernel.bin
	rm -rf master.img
	rm -rf master.img.lock

.PHONY: bochs
bochs: master.img
	/home/rist/bochs/bin/bochs -q -f bxrc
