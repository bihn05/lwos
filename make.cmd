echo off

:st

echo ---------------------Clean Stage

del /q master.img
del /q *.bin

echo ---------------------Base System
:com

nasm -f bin MBR.S -o MBR.BIN
nasm -f bin FAT.S -o FAT.BIN
nasm -f bin FDT.S -o FDT.BIN
nasm -f bin LWOS.S -o LWOS.BIN

set /p cs=Continue or Retry? (c/r)
if /i "%cs%" == "r" (
	goto com
)

echo ---------------------Generating Image

rem bximage -q -func=create -sectsize=512 -hd=16 master.img
copy img.img master.img

dd bs=512 count=1 seek=0 if=MBR.BIN of=master.img
dd bs=512 count=128 seek=1 if=FAT.BIN of=master.img
dd bs=512 count=128 seek=129 if=FDT.BIN of=master.img
dd bs=512 count=1 seek=257 if=WHEEL.BI_ of=master.img
dd bs=512 count=8 seek=258 if=BASFNT.FNT of=master.img
dd bs=512 count=2 seek=266 if=LWOS.BIN of=master.img

echo ---------------------Power On

bochs -q

echo ---------------------Emulation Terminated

echo on
