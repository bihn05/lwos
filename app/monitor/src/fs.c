#include "fs.h"
#include "mem.h"

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

void resolve_path(const char * path, PDWORD next) {
    ;
}

void fs_init(void) {
    lw_puts("FILESYSTEM DETECT\n\r");
    lw_disk_probe();
    PBLKDEV disk = lw_blk_get(0);
    if (!disk || disk->sector_size != sizeof(buf)) {
        lw_puts("NO SUPPORTED BLOCK DEVICE\n\r");
        return;
    }
    int err = disk->read(disk, 0, 1, buf);
    if (err) {
        lw_puts("MBR READ FAILED\n\r");
        return;
    }
    lw_dump128((PVOID)buf);

    memcpy(mbrpte, (PBYTE)buf + 0x1be, sizeof(mbrpte));


}