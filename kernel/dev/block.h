#ifndef _DEVICE_H_

#include <stdint.h>
#include <kernel.h>

typedef struct block_dev {
    char dev_name[32];
    uint32_t sector_size;
    uint64_t total_sectors;

    int (*read_block)(struct block_dev* dev, uint64_t lba, uint32_t count, void* buffer);
    int (*write_block)(struct block_dev* dev, uint64_t lba, uint32_t count, const void* buffer);

    void* private_data; // 用于存储设备特定的数据，如 ATA 设备的端口信息等
} block_dev_t;

#endif