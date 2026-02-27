# external/Makefile

CC = gcc
LD = ld
LDFLAGS = -n -Ttext 0x400000  # 典型的用户态起始虚拟地址

SRC = source/test.c
OBJ = build/test.elf

all: $(OBJ)

$(OBJ): $(SRC)
	$(CC) -I./include -m64 -nostdlib -fno-builtin -fno-pic -fno-pie -no-pie -static -e main -Ttext 0x400000 $< -o $@

clean:
	rm -rf build/*