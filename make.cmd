echo off

:st
echo ---------------------Clean Stage

del /q master.img
del /q *.bin
del /q system.map
del /q KERNEL\kernel.bin
del /q KERNEL\*.o

echo ---------------------Base System
:com

nasm -f bin MBR.S -o MBR.BIN
nasm -f bin FAT.S -o FAT.BIN
nasm -f bin FDT.S -o FDT.BIN
cd KERNEL
nasm -f elf32 START.S -o START.o
nasm -f elf32 schedule.S -o schedule.o
nasm -f elf32 handler.S -o handler.o
gcc -m32 -Wimplicit-function-declaration -fno-builtin -ffreestanding -nodefaultlibs -nostdinc -nostdlib -fno-pic -fno-pie -fno-stack-protector -I. -c main.c -o main.o
ld -m i386pe -static -o ./kernel.bin START.o schedule.o handler.o main.o -Ttext 0x70000 -L"./"
::gcc -m32 -static -o kernel.bin main.o START.o schedule.o -Ttext 0x70000 -nostdlib
cd..
objcopy -O binary ./KERNEL/kernel.bin system.bin
nm ./KERNEL/kernel.bin | sort > system.map

set /p cs=Continue , Failed or Retry? (c/f/r)
if /i "%cs%" == "r" (
	goto com
)
if /i "%cs%" == "f" (
	exit
)

echo ---------------------Generating Image

rem bximage -q -func=create -sectsize=512 -hd=16 master.img
copy img.img master.img

dd bs=512 count=1 seek=0 if=MBR.BIN of=master.img
dd bs=512 count=128 seek=1 if=FAT.BIN of=master.img
dd bs=512 count=128 seek=129 if=FDT.BIN of=master.img
dd bs=512 count=1 seek=257 if=WHEEL.BI_ of=master.img
dd bs=512 count=8 seek=258 if=BASFNT.FNT of=master.img
dd bs=512 count=128 seek=270 if=system.bin of=master.img

echo ---------------------Power On

bochs -q

echo ---------------------Emulation Terminated

echo on
