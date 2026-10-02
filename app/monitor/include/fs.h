#ifndef _FAT32_H
#define _FAT32_H

#include "abi.h"

typedef struct {
    BYTE flag; // 80h -> active, 00h -> inactive
    BYTE chs_beg[3]; // old method
    BYTE series; // partition series
    BYTE chs_end[3];
    DWORD lba;
    DWORD lba_count;
} MBR_T, *PMBR_T; // MBR partition table entry

void fs_init(void);

#endif
