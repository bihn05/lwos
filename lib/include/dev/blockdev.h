#ifndef _BLOCKDEV_H
#define _BLOCKDEV_H

#include "stdint.h"

typedef struct _BLKDEV {
    DWORD version, size;
    char name[8];
    DWORD sector_size;
    QWORD sectors;
    int (*read) (
        struct _BLKDEV *d,
        QWORD lba,
        DWORD count,
        PVOID buf
    );
    int (*write) (
        struct _BLKDEV *d,
        QWORD lba,
        DWORD count,
        PCVOID buf
    );
    int (*flush) (struct _BLKDEV *d);
    void (*poll) (struct _BLKDEV *d);
    PVOID priv;
} BLKDEV, *PBLKDEV;

#define BLKDEV_VERSION 1u
#define BLKDEV_MAX_DEVICES 16

/* 注册表由 ABI 管理; 对象及 priv 必须在使用期间一直有效。
 * reset/probe 仅在启动阶段调用, 不可与设备读写并发。
 */
void blkdev_reset(void);
int blkdev_register(PBLKDEV d);
BYTE blkdev_count(void);
PBLKDEV blkdev_get(BYTE index);

#endif