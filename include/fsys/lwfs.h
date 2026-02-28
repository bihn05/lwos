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

#define LWFAT32_EOC 	0xFFFFFFFF

#include <stdint.h>
#include <driver/ata.h>
#include <driver/block.h>
#include <fsys/vfs.h>
#include <vsprintf.h>

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
	uint8_t cluster_shift; 
	uint8_t flags;
	uint8_t drive_select;
	uint8_t used_percent;
	char reserved2;
} mbr_t;
#pragma pack(pop)
extern mbr_t sb;
#pragma pack(push, 1)
typedef struct {
	char filename[28];
	uint32_t length;
	char extname[3];
	uint8_t attr;
	uint32_t cluster_start;
	uint32_t parent_idx; // 父级目录在 FAT 表中的索引，根目录的 parent_idx 可以设为 0xFFFFFFFF 或者 0
	uint32_t first_child_idx; // 第一个子级目录在 FAT 表中的索引，如果没有子级则设为 0xFFFFFFFF 或者 0
	uint32_t sibling_idx; // 下一个同级目录在 FAT 表中的索引，如果没有同级则设为 0xFFFFFFFF 或者 0
	uint32_t reserved;
	uint64_t modify_time;
} fat_t;
#pragma pack(pop)

typedef struct {
    block_dev_t* disk;          // 绑定的块设备
    mbr_t sb;      // 内存中缓存的超级块
    vfs_node_t* root_node;      // 挂载点(根目录)的 VFS 节点
	uint8_t* cache_buf; // 用于读取 FAT 表和数据簇的缓存区
} lwfs_instance_t;

// 初始化并挂载 LWFS 文件系统
vfs_node_t* lwfs_mount(block_dev_t* dev);
static uint64_t lwfs_cluster_to_lba(lwfs_instance_t* inst, uint32_t cluster);
uint32_t lwfs_get_next_cluster(lwfs_instance_t* inst, uint32_t cluster);
int lwfs_read_cluster(lwfs_instance_t* inst, uint32_t lwfs_cluster, uint8_t* buffer);
uint32_t lwfs_get_file_size(lwfs_instance_t* inst, uint32_t start_cluster);
int lwfs_read_node(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
vfs_node_t* lwfs_finddir(struct vfs_node* dir_node, const char* target_name);
static int lwfs_read_fat_entry_by_idx(lwfs_instance_t* inst, uint32_t index, fat_t* out_entry);
void lwfs_format_filename(fat_t* emtry, char* out_name);
vfs_node_t* lwfs_resolve_path(lwfs_instance_t* inst, const char* path);
uint32_t lwfs_allocate_cluster(lwfs_instance_t* inst);
int lwfs_write_fat_entry_by_idx(lwfs_instance_t* inst, uint32_t index, fat_t* in_entry);
uint32_t lwfs_alloc_fat_entry(lwfs_instance_t* inst);
uint32_t lwfs_link_new_entry(lwfs_instance_t* inst, uint32_t parent_idx, fat_t* new_entry_data);
uint32_t lwfs_alloc_and_link_cluster(lwfs_instance_t* inst, uint32_t prev_cluster);
int lwfs_write_node(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
int lwfs_write_cluster(lwfs_instance_t* inst, uint32_t lwfs_cluster, uint8_t* buffer);
int lwfs_create(struct vfs_node* dir_node, const char* name, uint32_t flags);

#endif