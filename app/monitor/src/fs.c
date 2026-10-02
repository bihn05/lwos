#include "fs.h"

static DWORD buf[128];

static BYTE num_fat = 0;
static WORD fat_size = 0; // in sectors
static DWORD start_lba = 0;
static DWORD root_cluster = 0;
static WORD reserved_sector = 0;
static DWORD cluster_to_lba(DWORD c) {
    return (c - 2) + start_lba;
}

static MBR_T mbrpte[4];
static int part_entry = 0;

static void *memcpy(void *dst, void *src, int count)
{
    void * ret = dst;
    while (count--) {
        *(char *)dst = *(char *)src;
        dst = (char *)dst + 1;
        src = (char *)src + 1;
    }
    return ret;
}

void tokenize() {

}

void resolve_path(const char * path, PDWORD next) {
    ;
}

void fs_init(void) {
    lw_puts("FILESYSTEM DETECT\n\r");
    lw_disk_probe();
    lw_disk_read(0,0,1,(PVOID)buf);
    lw_dump128((PVOID)buf);

    memcpy(mbrpte, buf+0x1be, 16*4);


}