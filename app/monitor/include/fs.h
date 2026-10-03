#ifndef _FAT32_H
#define _FAT32_H

#include "abi.h"

typedef struct _MBR_T{
    BYTE flag; // 80h -> active, 00h -> inactive
    BYTE chs_beg[3]; // old method
    BYTE series; // partition series
    BYTE chs_end[3];
    DWORD lba;
    DWORD lba_count;
} MBR_T, *PMBR_T; // MBR partition table entry
typedef struct _SFT_T{
    CHAR name[12]; // 8.3 磁盘字段为 11 字节, 另留 NUL
    BYTE attr;
    DWORD cluster;
    DWORD length;
} SFT_T, *PSFT_T; // simplified file table entries
enum {
    FS_ENTRY_END = 0u,
    FS_ENTRY_VALID = 1u,
    FS_ENTRY_SKIP = 2u,
    FS_PATH_ERROR = -1u,
    FS_ERR_FORMAT = -2u,
    FS_ERR_IO = -4u,
    FS_ERR_NOT_MOUNTED = -5u
};

int fs_init(void);
int resolve_path(PCSTR path, PDWORD cluster);
// read fte from input index
// -5  not mounted
// -2  problem
//  0  end
//  1  valid
//  2  skip
int fat_read_fte(DWORD dir_cluster, int index, PSFT_T p);
int dir_find(DWORD dir_cluster, PCSTR name, PSFT_T out, int is_dir);
void fs_scan(void);
#endif
