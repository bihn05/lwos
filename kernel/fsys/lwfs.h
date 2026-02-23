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
#include <fsys/block.h>
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
mbr_t sb;
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

// 前置声明：LWFS 专门实现的读取函数和查找函数
static int lwfs_read_node(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
static vfs_node_t* lwfs_finddir(vfs_node_t* dir, const char* name);

static vfs_ops_t lwfs_vfs_ops = {
    .read = lwfs_read_node,
    .write = NULL, // 待实现
    .finddir = lwfs_finddir
};

vfs_node_t* lwfs_mount(block_dev_t* dev) {
	if (!dev || !dev->ops || !dev->ops->read) {
		printk("Invalid block device for LWFS mount\n");
		return NULL;
	}
	printk("Mounting LWFS on device: %s\n", dev->dev_name);

    lwfs_instance_t* inst = (lwfs_instance_t*)kmalloc(sizeof(lwfs_instance_t));
    inst->disk = dev;
	uint32_t max_buf_size = dev->sector_size * (1 << 3);
	inst->cache_buf = (uint8_t*)kmalloc(max_buf_size); // 分配足够大的缓存区来读取一个簇的数据
    // 1. 读取超级块 (MBR 偏移为 0 的地方)
    // 注意：要跳过前几个字节的汇编指令，准确读取结构体
    int res = dev->ops->read(dev, 0, 1, inst->cache_buf);
	if (res != 0) {
		printk("Failed to read MBR for LWFS mount\n");
		kfree(inst->cache_buf);
		kfree(inst);
		return NULL;
	}
	printk("MBR read successfully for LWFS mount\n");

	memcpy(&inst->sb, inst->cache_buf, sizeof(mbr_t));
	if (strncmp(inst->sb.filesys_name, "LWFAT32", 7) != 0) {
		printk("Invalid filesystem signature: %s\n", inst->sb.filesys_name);
		kfree(inst->cache_buf);
		kfree(inst);
		return NULL;
	}
	printk("LWFS MBR parsed successfully: root cluster = %d, total clusters = %d\n",
		inst->sb.root_cluster, inst->sb.cluster_count);

    // 2. 创建并组装根目录的 VFS 节点
    vfs_node_t* root_node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
	strcpy(root_node->name, "/");
	root_node->flags = VFS_FLAG_DIR;
	root_node->size = 0;
    
    root_node->fs_instance = inst;
	root_node->internal = (uint32_t)inst->sb.root_cluster; // 将根目录的起始簇号存到 fs_private_data
	root_node->ops = &lwfs_vfs_ops;

	printk("Root cluster: %d\n", inst->sb.root_cluster);

    return root_node;
}

static uint64_t lwfs_cluster_to_lba(lwfs_instance_t* inst, uint32_t cluster) {
	// 8200 + 512 + 27*8 = 8928
	uint32_t data_start_lba = inst->sb.cluster_offset + inst->sb.cluster_count; // 数据区起始 LBA
	printk("clstoff %d, clstcnt %d, clstshf %d, clst %d\n", inst->sb.cluster_offset, inst->sb.cluster_count, inst->sb.cluster_shift, cluster);
	return data_start_lba + (cluster) * (1 << inst->sb.cluster_shift);
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
int lwfs_read_cluster(lwfs_instance_t* inst, uint32_t lwfs_cluster, uint8_t* buffer) {
	printk("Reading cluster %d from disk...\n", lwfs_cluster);
	uint64_t lba = lwfs_cluster_to_lba(inst, lwfs_cluster);
	printk("so it is lba %d\n", lba);
	uint32_t sectors_per_cluster = 1 << (inst->sb.cluster_shift);
	return inst->disk->ops->read(inst->disk, lba, sectors_per_cluster, buffer);
}
uint32_t lwfs_get_file_size(lwfs_instance_t* inst, uint32_t start_cluster) {
	uint32_t size = 0;
	uint32_t cluster = start_cluster;

	while (cluster != LWFAT32_EOC) {
		size += (1 << inst->sb.cluster_shift) * inst->disk->sector_size;
		cluster = lwfs_get_next_cluster(inst, cluster);
	}

	return size;
}
int lwfs_read_node(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
	lwfs_instance_t* inst = (lwfs_instance_t*)node->fs_instance;
	uint32_t current_cluster = (uint32_t)node->fs_private_data;

	if (offset >= node->size) return 0; // 偏移超出文件大小
	if (offset + size > node->size) size = node->size - offset; // 调整读取大小

	uint32_t cluster_size = (1 << inst->sb.cluster_shift) * inst->disk->sector_size;
	uint32_t cluster_index = offset / cluster_size;
	uint32_t offset_in_cluster = offset % cluster_size;

	// 定位到正确的簇
	for (uint32_t i = 0; i < cluster_index; i++) {
		current_cluster = lwfs_get_next_cluster(inst, current_cluster);
		if (current_cluster == LWFAT32_EOC) return 0; // 文件结束
	}

	// 从当前簇开始读取数据
	int bytes_read = 0;
	while (bytes_read < size) {
		if (current_cluster == LWFAT32_EOC) break; // 文件结束

		lwfs_read_cluster(inst, current_cluster, inst->cache_buf);

		uint32_t to_copy = cluster_size - offset_in_cluster;
		if (to_copy > (size - bytes_read)) to_copy = size - bytes_read;

		memcpy(buffer + bytes_read, inst->cache_buf + offset_in_cluster, to_copy);
		bytes_read += to_copy;

		offset_in_cluster = 0; // 后续簇从头开始读
		current_cluster = lwfs_get_next_cluster(inst, current_cluster);
	}

	return bytes_read;
}

vfs_node_t* lwfs_finddir(struct vfs_node* dir_node, const char* target_name) {
    if (!(dir_node->flags & VFS_FLAG_DIR)) {
        printk("Node %s is not a directory\n", dir_node->name);
        return NULL;
    }

    lwfs_instance_t* inst = (lwfs_instance_t*)dir_node->fs_instance;
    fat_t* entry = (fat_t*)inst->cache_buf; // 直接把缓存区当作 fat_t 数组来解析，每次读取一个扇区（8 个 fat_t 条目）

    // 获取 FAT 区（文件安排表）的起始扇区 (LBA) 和总扇区数
    uint32_t fat_start_lba = inst->sb.fat_offset;
    uint32_t fat_sectors_count = inst->sb.fat_length;
    uint32_t entries_per_sector = inst->disk->sector_size / sizeof(fat_t); // 512 / 64 = 8

    printk("Searching for '%s' in FAT area (Start LBA: %d, Sectors: %d)\n", 
           target_name, fat_start_lba, fat_sectors_count);

    // 逐个扇区遍历 FAT 区域
    for (uint32_t sector_idx = 0; sector_idx < fat_sectors_count; sector_idx++) {
        
        // 每次读取 1 个扇区到缓存区
        int read_res = inst->disk->ops->read(inst->disk, fat_start_lba + sector_idx, 1, inst->cache_buf);
        if (read_res != 0) {
            printk("Disk read error at LBA %d\n", fat_start_lba + sector_idx);
            return NULL;
        }

        // 遍历该扇区内的所有目录项 (0 ~ 7)
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            // 文件名首字节为 \0 代表该表项未被使用
            if (entry[i].filename[0] == '\0') {
                continue; 
            }

            char full_name[36];
            memset(full_name, 0, sizeof(full_name));
            
            // 1. 拷贝文件名并去除尾部空格
            strncpy(full_name, entry[i].filename, 28);
            for (int j = 27; j >= 0; j--) {
                if (full_name[j] == ' ') full_name[j] = '\0';
                else if (full_name[j] != '\0') break;
            }

            // 2. 拷贝扩展名并去除尾部空格
            if (entry[i].extname[0] != '\0' && entry[i].extname[0] != ' ') {
                strncat(full_name, ".", 1);
                
                char temp_ext[4];
                memset(temp_ext, 0, 4);
                strncpy(temp_ext, entry[i].extname, 3);
                for (int j = 2; j >= 0; j--) {
                    if (temp_ext[j] == ' ') temp_ext[j] = '\0';
                    else break;
                }
                strncat(full_name, temp_ext, 3);
            }

            // 打印出清理干净的文件名，方便调试
            // printk("Checking: [%s] (Cluster: %d, Size: %d)\n", full_name, entry[i].cluster_start, entry[i].length);

            // 3. 匹配文件名
            if (strcmp(full_name, target_name) == 0) {
                vfs_node_t* found_node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
                memset(found_node, 0, sizeof(vfs_node_t));
                
                strncpy(found_node->name, full_name, 31);
                found_node->size = entry[i].length;
                found_node->flags = (entry[i].attr & FILE_DIR) ? VFS_FLAG_DIR : VFS_FLAG_FILE;
                found_node->fs_instance = inst;
                
                // 核心：虽然在 FAT 区查找用的是扇区逻辑，但文件的【实际数据】依然存放在簇堆区。
                // 所以我们必须把 cluster_start 存入 fs_private_data，交还给 read_node 去读簇。
                found_node->fs_private_data = (void*)entry[i].cluster_start; 
                found_node->ops = &lwfs_vfs_ops;
                
                return found_node;
            }
        }
    }
    
    return NULL; // 遍历完所有 FAT 扇区仍未找到
}

static int lwfs_read_fat_entry_by_idx(lwfs_instance_t* inst, uint32_t index, fat_t* out_entry) {
	if (index == 0xFFFFFFFF) {
		return -1;
	}
	uint32_t entries_per_sector = inst->disk->sector_size / sizeof(fat_t);
	uint32_t sector_lba = inst->sb.fat_offset + (index / entries_per_sector);
	uint32_t offset = index % entries_per_sector;

	if (inst->disk->ops->read(inst->disk, sector_lba, 1, inst->cache_buf) != 0) {
		printk("Disk read error at LBA %d\n", sector_lba);
		return -1;
	}

	memcpy(out_entry, inst->cache_buf + offset * sizeof(fat_t), sizeof(fat_t));
	printk("FAT entry at index %d: filename [%s], attr 0x%02X, cluster %d, length %d\n", 
		index, out_entry->filename, out_entry->attr, out_entry->cluster_start, out_entry->length);

	return 0;
}
void lwfs_format_filename(fat_t* emtry, char* out_name) {
	memset(out_name, 0, 36);
	strncpy(out_name, emtry->filename, 28);
	for (int j = 27; j >= 0; j--) {
		if (out_name[j] == ' ') out_name[j] = '\0';
		else if (out_name[j] != '\0') break;
	}
	
	char temp_ext[4];
	memset(temp_ext, 0, 4);
	strncpy(temp_ext, emtry->extname, 3);
	for (int j = 2; j >= 0; j--) {
		if (temp_ext[j] == ' ') temp_ext[j] = '\0';
		else break;
	}

	if (temp_ext[0] != '\0') {
		strncat(out_name, ".", 1);
		strncat(out_name, temp_ext, 3);
	}
}
vfs_node_t* lwfs_resolve_path(lwfs_instance_t* inst, const char* path) {
    if (!path || path[0] != '/') return NULL; // 目前仅支持绝对路径

    // 假设根目录在 FAT 表的第 0 个索引
    uint32_t current_idx = 0; 
    fat_t current_entry;
    
    if (lwfs_read_fat_entry_by_idx(inst, current_idx, &current_entry) != 0) {
		printk("Failed to read root directory entry from FAT\n");
        return NULL;
    }

    char target_name[32];
    int path_pos = 1; // 跳过开头的 '/'

    // 逐级解析路径
    while (path[path_pos] != '\0') {
        // 1. 提取当前层级的名字 (例如从 /usr/bin 提取出 usr)
        int name_len = 0;
        memset(target_name, 0, sizeof(target_name));
        while (path[path_pos] != '/' && path[path_pos] != '\0' && name_len < 31) {
            target_name[name_len++] = path[path_pos++];
        }
        if (path[path_pos] == '/') path_pos++; // 跳过多余的斜杠

        // 2. 必须是目录才能下潜
        if (!(current_entry.attr & FILE_DIR)) return NULL;

        // 3. 在当前目录下，顺着兄弟链表寻找 target_name
        uint32_t child_idx = current_entry.first_child_idx;
        int found = 0;

        while (child_idx != 0xFFFFFFFF) {
            fat_t child_entry;
            if (lwfs_read_fat_entry_by_idx(inst, child_idx, &child_entry) != 0) break;

            // 这里可以复用你之前写的清洗空格和拼接扩展名的逻辑
            if (current_entry.filename[0] == '\0') {
				printk("Skipping empty FAT entry at index %d\n", child_idx);
				child_idx = child_entry.sibling_idx;
				continue; 
			}

			char formatted_name[36];
			lwfs_format_filename(&child_entry, formatted_name);
			printk("Checking: [%s] (Cluster: %d, Size: %d)\n", formatted_name, child_entry.cluster_start, child_entry.length);

            if (strcmp(formatted_name, target_name) == 0) {
                // 找到了！更新当前节点，准备进入下一级循环
                current_idx = child_idx;
                current_entry = child_entry;
                found = 1;
                break;
            }
            // 没找到，看下一个兄弟节点
            child_idx = child_entry.sibling_idx;
        }

        if (!found) return NULL; // 该层级路径不存在
    }

    // 循环结束，current_entry 就是目标文件/目录
    // 此时将其组装为 vfs_node_t 交给内核即可
    vfs_node_t* node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    // ... 赋值逻辑 ...
	node->size = current_entry.length;
	node->flags = (current_entry.attr & FILE_DIR) ? VFS_FLAG_DIR : VFS_FLAG_FILE;
	node->fs_instance = inst;
    node->fs_private_data = (void*)current_entry.cluster_start;
	strncpy(node->name, target_name, 31);
	node->ops = &lwfs_vfs_ops;
    
    return node;
}
#endif