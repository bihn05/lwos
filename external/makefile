# external/Makefile

CC = gcc

SRC = source/test.c
OBJ = build/test.elf

all: $(OBJ)

$(OBJ): $(SRC)
	$(CC) -I./include -m64 -nostdlib -fno-builtin -fno-pic -fno-pie -no-pie -static -e main -Ttext 0x400000 $< -o $@

clean:
	rm -rf build/*