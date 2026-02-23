// kernel/fsys/block.h

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include <stdint.h>
#include <kernel.h>

struct block_dev;

typedef struct {
    int (*read)(struct block_dev* dev, uint64_t lba, uint32_t count, void* buffer);
    int (*write)(struct block_dev* dev, uint64_t lba, uint32_t count, void* buffer);
    void (*get_info)(struct block_dev* dev);
} block_dev_ops_t;

typedef struct block_dev {
    char dev_name[32];
    uint32_t sector_size;
    uint64_t total_sectors;

    block_dev_ops_t* ops;
    void* private_data; // 用于存储设备特定的数据，如 ATA 设备的端口信息等
} block_dev_t;

#endif