// kernel/fsys/lwfs.h
#ifndef _LWFS_H
#define _LWFS_H

#define FILE_RO			0x01
#define FILE_SYSTEM		0x02
#define FILE_ARCHIVE	0x04
#define FILE_DIR		0x08
#define FILE_PRESENT	0x80

#define VOL_LOW_SPEED	0x01
#define VOL_BAD			0x02
#define VOL_BOOTABLE	0x04

#include <stdint.h>
#include <driver/ata.h>
#include <fsys/block.h>
#include <fsys/vfs.h>

#pragma pack(push, 1)
typedef struct {
	uint8_t jmp_code[3];
	char filesys_name[8];
	char reserved[5];
	uint64_t partition_offset;
	uint64_t partition_length;
	uint32_t fat_offset;
	uint32_t fat_length;
	uint32_t cluster_offset;
	uint32_t cluster_count;
	uint32_t root_cluster; 
	uint32_t volume_serial_number;
	uint16_t fs_version;
	uint8_t sector_shift;
	uint8_t clustor_shift; 
	uint8_t flags;
	uint8_t drive_select;
	uint8_t used_percent;
	char reserved2;
} mbr_t;
#pragma pack(pop)
mbr_t sb;
#pragma pack(push, 1)
typedef struct {
	char filename[28];
	uint32_t length;
	char extname[3];
	uint8_t attr;
	uint32_t cluster_start;
	uint32_t reserved;
	uint32_t reserved2;
	uint64_t time_create;
	uint64_t time_modify;
} fat_t;
#pragma pack(pop)

typedef struct {
    block_dev_t* disk;          // 绑定的块设备
    mbr_t sb;      // 内存中缓存的超级块
    vfs_node_t* root_node;      // 挂载点(根目录)的 VFS 节点
} lwfs_instance_t;

// 初始化并挂载 LWFS 文件系统
lwfs_instance_t* lwfs_mount(block_dev_t* dev);

// 前置声明：LWFS 专门实现的读取函数和查找函数
static int lwfs_read_file(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
static vfs_node_t* lwfs_finddir(vfs_node_t* dir, const char* name);

static vfs_ops_t lwfs_vfs_ops = {
    .read = lwfs_read_file,
    .write = NULL, // 待实现
    .finddir = lwfs_finddir
};

lwfs_instance_t* lwfs_mount(block_dev_t* dev) {
    lwfs_instance_t* inst = (lwfs_instance_t*)kmalloc(sizeof(lwfs_instance_t));
    inst->disk = dev;
    // 1. 读取超级块 (MBR 偏移为 0 的地方)
    // 注意：要跳过前几个字节的汇编指令，准确读取结构体
    dev->ops->read(dev, 0, 1, &sb);

    // 2. 创建并组装根目录的 VFS 节点
    inst->root_node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    strcpy(inst->root_node->name, "/");
    inst->root_node->flags = VFS_FLAG_DIR;
    
    // 利用私有数据存放起始簇号，假设从超级块的 root_cluster 获取
    inst->root_node->fs_private_data = (void*)inst->sb.root_cluster; 
    
    // 3. 将 VFS 的操作指针，指向 LWFS 的具体实现
    inst->root_node->ops = &lwfs_vfs_ops;

    return inst;
}

static uint64_t lwfs_cluster_to_lba(lwfs_instance_t* inst, uint32_t cluster) {
	return inst->sb.cluster_offset + (cluster - 2) * (1 << inst->sb.clustor_shift);
}
uint32_t lwfs_get_next_cluster(lwfs_instance_t* inst, uint32_t cluster) {
	uint32_t fat_lba = inst->sb.cluster_offset;
	uint32_t entries_per_sector = inst->disk->sector_size / sizeof(uint32_t);

	uint32_t sector_idx = cluster / entries_per_sector;
	uint32_t offset_in_sector = cluster % entries_per_sector;

	uint32_t* heap_buf = (uint32_t*)inst->cache_buf;
	inst->disk->ops->read(inst->disk, fat_lba + sector_idx, 1, heap_buf);

	return heap_buf[offset_in_sector];
}

#endif