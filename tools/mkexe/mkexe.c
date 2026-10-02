#include <stdio.h>
#include <stdint.h>

typedef struct {
    unsigned int magic;
    unsigned short version;
    unsigned short hdr_size;
    unsigned int flags;
    unsigned int abi_need;

    unsigned int entry;
    unsigned int image_off;
    unsigned int image_size;
    unsigned int mem_size;

    unsigned int stack_size;
    unsigned int reloc_off;
    unsigned int reloc_count;
    unsigned int sym_off;

    unsigned int sym_count;
    unsigned int export_off;
    unsigned int export_count;
    unsigned int crc32;
} LwpHdr;

int main() {

}