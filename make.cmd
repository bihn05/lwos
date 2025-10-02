echo off

:st
echo ---------------------Clean Stage

del /q master.img
del /q *.bin
del /q system.map
del /q kernel\kernel.bin
del /q kernel\*.o
del /q kernel\mm\*.o
del /q loader\*.o
del /q loader\*.bin

echo ---------------------Base System
:com

nasm -f bin mbr.s -o mbr.bin
nasm -f bin loader.s -o loader.bin
nasm -f bin oempara.s -o oempara.bin

cd kernel
nasm -f elf32 start.s -o start.o
nasm -f elf32 int.s -o int.o
nasm -f elf32 segment.s -o segment.o
nasm -f elf32 mm\schedule.s -o mm\schedule.o
gcc -m32 -Wimplicit-function-declaration -w -fno-builtin -ffreestanding -nodefaultlibs -nostdinc -nostdlib -fno-pic -fno-pie -fno-stack-protector -I. -c main.c -o main.o
ld -m i386pe -static -o ./kernel.bin start.o main.o int.o segment.o mm/schedule.o -T linker.ld -L"./"

cd..
objcopy -O binary ./kernel/kernel.bin system.bin
rem nm ./kernel/kernel.bin | sort > system.map

set /p cs=Continue , Failed or Retry? (c/f/r)
if /i "%cs%" == "r" (
	goto com
)
if /i "%cs%" == "f" (
	goto end
)

echo ---------------------Generating Image

rem bximage -q -func=create -sectsize=512 -hd=16 master.img
copy img.img master.img

dd bs=512 count=1 seek=0 if=mbr.bin of=master.img
dd bs=512 count=8 seek=1 if=loader.bin of=master.img
dd bs=512 count=80 seek=261 if=system.bin of=master.img

copy master.img vdsk.img

echo ---------------------Power On

if "%1" == "-n" (
	goto end
)

if "%1" == "-d" (
	bochsdbg -q
) else (
	bochs -q
)

echo ---------------------Emulation Terminated

:end

echo on
