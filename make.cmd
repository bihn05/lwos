echo off

echo ---------------------Clean Stage

del /q master.img
del /q *.bin

echo ---------------------Base System

nasm -f bin MBR.S -o MBR.BIN
nasm -f bin FAT.S -o FAT.BIN
nasm -f bin FDT.S -o FDT.BIN
nasm -f bin LWOS.S -o LWOS.BIN

pause

echo ---------------------Generating Image

bximage -q -func=create -sectsize=512 -hd=16 master.img

dd bs=512 count=1 seek=0 if=MBR.BIN of=master.img
dd bs=512 count=128 seek=1 if=FAT.BIN of=master.img
dd bs=512 count=128 seek=129 if=FDT.BIN of=master.img
dd bs=512 count=1 seek=257 if=WHEEL.BI_ of=master.img
dd bs=512 count=1 seek=258 if=LWOS.BIN of=master.img

pause

echo ---------------------Power On

bochs -q

echo ---------------------Emulation Terminated

echo on
