#include <fsys/lwfs.h>

mbr_t sb;

static vfs_ops_t lwfs_vfs_ops = {
    .read = lwfs_read_node,
    .write = lwfs_write_node,
    .finddir = lwfs_finddir,
    .create = lwfs_create
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
	// printk("clstoff %d, clstcnt %d, clstshf %d, clst %d\n", inst->sb.cluster_offset, inst->sb.cluster_count, inst->sb.cluster_shift, cluster);
	return data_start_lba + (cluster) * (1 << inst->sb.cluster_shift);
}
uint32_t lwfs_get_next_cluster(lwfs_instance_t* inst, uint32_t cluster) {
	uint32_t cluster_lba = inst->sb.cluster_offset;
	uint32_t entries_per_sector = inst->disk->sector_size / sizeof(uint32_t);

	uint32_t sector_idx = cluster / entries_per_sector;
	uint32_t offset_in_sector = cluster % entries_per_sector;

	uint32_t* heap_buf = (uint32_t*)inst->cache_buf;
	inst->disk->ops->read(inst->disk, cluster_lba + sector_idx, 1, heap_buf);

	return heap_buf[offset_in_sector];
}
int lwfs_read_cluster(lwfs_instance_t* inst, uint32_t lwfs_cluster, uint8_t* buffer) {
	// printk("Reading cluster %d from disk...\n", lwfs_cluster);
	uint64_t lba = lwfs_cluster_to_lba(inst, lwfs_cluster);
	// printk("so it is lba %d\n", lba);
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
                found_node->internal = (sector_idx * entries_per_sector) + i;
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
    node->internal = current_idx;
	strncpy(node->name, target_name, 31);
	node->ops = &lwfs_vfs_ops;
    
    return node;
}
uint32_t lwfs_allocate_cluster(lwfs_instance_t* inst) {
    uint32_t fat_entries_per_sector = inst->disk->sector_size / sizeof(uint32_t);
    uint32_t* fat_buf = (uint32_t*)inst->cache_buf;

    for (uint32_t i = 0; i < inst->sb.fat_length; i++) {
        inst->disk->ops->read(inst->disk, inst->sb.fat_offset + i, 1, (uint8_t*)fat_buf);
        for (uint32_t j = 0; j < fat_entries_per_sector; j++) {
            if (fat_buf[j] == 0) { // 找到空闲簇
                fat_buf[j] = LWFAT32_EOC;
                inst->disk->ops->write(inst->disk, inst->sb.fat_offset + i, 1, (uint8_t*)fat_buf);
                return i * fat_entries_per_sector + j;
            }
        }
    }
    return 0; // 磁盘满
}
int lwfs_write_fat_entry_by_idx(lwfs_instance_t* inst, uint32_t index, fat_t* in_entry) {
    if (index == 0xFFFFFFFF || in_entry == NULL) {
        return -1;
    }

    uint32_t entries_per_sector = inst->disk->sector_size / sizeof(fat_t);
    uint32_t sector_lba = inst->sb.fat_offset + (index / entries_per_sector);
    uint32_t offset = index % entries_per_sector;

    if (inst->disk->ops->read(inst->disk, sector_lba, 1, inst->cache_buf) != 0) {
        printk("Disk read error at LBA %d during FAT entry update\n", sector_lba);
        return -1;
    }

    memcpy(inst->cache_buf + (offset * sizeof(fat_t)), in_entry, sizeof(fat_t));

    if (inst->disk->ops->write(inst->disk, sector_lba, 1, inst->cache_buf) != 0) {
        printk("Disk write error at LBA %d during FAT entry update\n", sector_lba);
        return -1;
    }

    printk("Successfully wrote FAT entry at index %d: filename [%s], length %d\n", 
           index, in_entry->filename, in_entry->length);

    return 0;
}
uint32_t lwfs_alloc_fat_entry(lwfs_instance_t* inst) {
    uint32_t fat_sectors_count = inst->sb.fat_length;
    uint32_t entries_per_sector = inst->disk->sector_size / sizeof(fat_t);
    fat_t* entry_buf = (fat_t*)inst->cache_buf;

    for (uint32_t sector_idx = 0; sector_idx < fat_sectors_count; sector_idx++) {
        inst->disk->ops->read(inst->disk, inst->sb.fat_offset + sector_idx, 1, (uint8_t*)entry_buf);
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            if (entry_buf[i].filename[0] == '\0') {
                // 找到空表项，返回其绝对索引
                return (sector_idx * entries_per_sector) + i;
            }
        }
    }
    return 0xFFFFFFFF; // FAT 表已满
}
uint32_t lwfs_link_new_entry(lwfs_instance_t* inst, uint32_t parent_idx, fat_t* new_entry_data) {
    // 1. 获取一个新的空闲 FAT 索引
    uint32_t new_idx = lwfs_alloc_fat_entry(inst);
    if (new_idx == 0xFFFFFFFF) {
        printk("Error: FAT table is full!\n");
        return 0xFFFFFFFF;
    }

    // 2. 初始化新节点的链表指针
    new_entry_data->parent_idx = parent_idx;
    new_entry_data->first_child_idx = 0xFFFFFFFF; // 新节点默认没有子节点
    new_entry_data->sibling_idx = 0xFFFFFFFF;     // 新节点默认没有下一个兄弟

    // 3. 读取父节点信息
    fat_t parent_entry;
    if (lwfs_read_fat_entry_by_idx(inst, parent_idx, &parent_entry) != 0) {
        return 0xFFFFFFFF;
    }

    // 4. 维护链表关系
    if (parent_entry.first_child_idx == 0xFFFFFFFF || parent_entry.first_child_idx == 0) {
        // 情况 A：父目录当前是空的，新节点是它的第一个孩子
        parent_entry.first_child_idx = new_idx;
        // 写回更新后的父节点
        lwfs_write_fat_entry_by_idx(inst, parent_idx, &parent_entry);
    } else {
        // 情况 B：父目录已经有孩子了，顺着藤摸瓜找到最后一个兄弟节点
        uint32_t current_sibling_idx = parent_entry.first_child_idx;
        fat_t current_sibling;
        
        while (1) {
            if (lwfs_read_fat_entry_by_idx(inst, current_sibling_idx, &current_sibling) != 0) {
                return 0xFFFFFFFF; // 读取兄弟节点出错
            }
            
            if (current_sibling.sibling_idx == 0xFFFFFFFF || current_sibling.sibling_idx == 0) {
                // 找到了最后一个兄弟节点，把它的 next 指向我们的新节点
                current_sibling.sibling_idx = new_idx;
                // 将修改后的兄弟节点写回磁盘
                lwfs_write_fat_entry_by_idx(inst, current_sibling_idx, &current_sibling);
                break;
            }
            // 继续找下一个兄弟
            current_sibling_idx = current_sibling.sibling_idx;
        }
    }

    // 5. 将配置好的新节点正式写入磁盘
    if (lwfs_write_fat_entry_by_idx(inst, new_idx, new_entry_data) != 0) {
        return 0xFFFFFFFF;
    }

    printk("Successfully linked new entry at index %d under parent %d\n", new_idx, parent_idx);
    return new_idx; // 返回新节点的索引，方便后续存入 vfs_node->internal
}
uint32_t lwfs_alloc_and_link_cluster(lwfs_instance_t* inst, uint32_t prev_cluster) {
    uint32_t fat_lba = inst->sb.fat_offset; 
    uint32_t entries_per_sector = inst->disk->sector_size / sizeof(uint32_t);
    uint32_t* fat_buf = (uint32_t*)kmalloc(inst->disk->sector_size);
    uint32_t new_cluster = 0xFFFFFFFF;

    // 1. 遍历 FAT 表寻找空闲簇 (值为 0)
    // 注意：通常簇 0 和簇 1 是保留的，数据簇从 2 开始
    for (uint32_t i = 0; i < inst->sb.fat_length; i++) {
        inst->disk->ops->read(inst->disk, fat_lba + i, 1, (uint8_t*)fat_buf);
        for (uint32_t j = 0; j < entries_per_sector; j++) {
            if (i == 0 && j < 2) continue; // 跳过保留簇
            
            if (fat_buf[j] == 0) { // 找到空闲簇
                new_cluster = i * entries_per_sector + j;
                fat_buf[j] = LWFAT32_EOC; // 标记为链条末尾
                inst->disk->ops->write(inst->disk, fat_lba + i, 1, (uint8_t*)fat_buf);
                break;
            }
        }
        if (new_cluster != 0xFFFFFFFF) break;
    }

    // 2. 如果存在前置簇，将前置簇的 FAT 表项指向新簇
    if (new_cluster != 0xFFFFFFFF && prev_cluster != LWFAT32_EOC && prev_cluster != 0) {
        uint32_t prev_sector = prev_cluster / entries_per_sector;
        uint32_t prev_offset = prev_cluster % entries_per_sector;
        
        inst->disk->ops->read(inst->disk, fat_lba + prev_sector, 1, (uint8_t*)fat_buf);
        fat_buf[prev_offset] = new_cluster;
        inst->disk->ops->write(inst->disk, fat_lba + prev_sector, 1, (uint8_t*)fat_buf);
    }

    kfree(fat_buf);
    return new_cluster;
}
int lwfs_write_cluster(lwfs_instance_t* inst, uint32_t lwfs_cluster, uint8_t* buffer) {
    // 1. 计算该簇对应的起始 LBA (逻辑块地址)
    // 逻辑：数据起始地址 + (簇号 * 每个簇的扇区数)
    uint64_t lba = lwfs_cluster_to_lba(inst, lwfs_cluster);
    
    // 2. 计算每个簇包含多少个扇区
    // 基于 sb.cluster_shift 得到 2^n 个扇区
    uint32_t sectors_per_cluster = 1 << (inst->sb.cluster_shift);

    // 3. 调用块设备驱动的写入接口
    // 参数：设备指针, 起始 LBA, 要写的扇区总数, 源数据缓冲区
    if (inst->disk->ops->write(inst->disk, lba, sectors_per_cluster, buffer) != 0) {
        printk("LWFS Error: Failed to write cluster %d at LBA %d\n", lwfs_cluster, lba);
        return -1;
    }

    return 0;
}
int lwfs_write_node(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    lwfs_instance_t* inst = (lwfs_instance_t*)node->fs_instance;
    uint32_t current_cluster = (uint32_t)node->fs_private_data;

    // 1. 偏移越界检查 (为了简单起见，目前不支持稀疏文件，必须连续写入)
    if (offset > node->size) {
        printk("Write offset exceeds file size. Sparse files not supported.\n");
        return 0; 
    }

    uint32_t cluster_size = (1 << inst->sb.cluster_shift) * inst->disk->sector_size;
    uint32_t cluster_index = offset / cluster_size;
    uint32_t offset_in_cluster = offset % cluster_size;

    // 2. 如果是空文件写入第一笔数据，分配起始簇
    if (current_cluster == LWFAT32_EOC || current_cluster == 0) {
        current_cluster = lwfs_alloc_and_link_cluster(inst, LWFAT32_EOC);
        if (current_cluster == 0xFFFFFFFF) return 0; // 磁盘已满
        node->fs_private_data = (void*)current_cluster; // 更新内存中的起始簇
    }

    uint32_t prev_cluster = LWFAT32_EOC;
    uint32_t iter_cluster = (uint32_t)node->fs_private_data;

    // 3. 顺藤摸瓜：遍历簇链，定位到 offset 所在的起始簇
    for (uint32_t i = 0; i < cluster_index; i++) {
        prev_cluster = iter_cluster;
        iter_cluster = lwfs_get_next_cluster(inst, iter_cluster);
        
        // 如果文件现有簇不够长，需要中途扩容
        if (iter_cluster == LWFAT32_EOC) {
            iter_cluster = lwfs_alloc_and_link_cluster(inst, prev_cluster);
            if (iter_cluster == 0xFFFFFFFF) return 0;
        }
    }
    current_cluster = iter_cluster;

    // 4. 核心写入循环
    int bytes_written = 0;
    // 使用独立的 buffer 避免覆盖 inst->cache_buf 中可能存在的元数据
    uint8_t* bounce_buf = (uint8_t*)kmalloc(cluster_size); 

    while (bytes_written < size) {
        // 如果当前簇是结尾，说明需要分配新簇继续写
        if (current_cluster == LWFAT32_EOC) {
            current_cluster = lwfs_alloc_and_link_cluster(inst, prev_cluster);
            if (current_cluster == 0xFFFFFFFF) break; // 磁盘已满，能写多少是多少
        }

        uint32_t to_write = cluster_size - offset_in_cluster;
        if (to_write > (size - bytes_written)) to_write = size - bytes_written;

        // 读-改-写 机制：如果不是覆盖写入整个簇，必须先把原来的簇数据读出来
        if (to_write < cluster_size) {
            lwfs_read_cluster(inst, current_cluster, bounce_buf);
        }

        // 把用户要写入的数据拷贝到 bounce_buf 的指定偏移处
        memcpy(bounce_buf + offset_in_cluster, buffer + bytes_written, to_write);
        
        // 将修改好的簇写回磁盘 (调用你之前写好的函数)
        lwfs_write_cluster(inst, current_cluster, bounce_buf);

        bytes_written += to_write;
        offset_in_cluster = 0; // 后续簇必然从头开始写

        prev_cluster = current_cluster;
        current_cluster = lwfs_get_next_cluster(inst, current_cluster);
    }
    kfree(bounce_buf);

    // 5. 更新文件元数据 (只有文件变大或者首次写入时才需要)
    if (offset + bytes_written > node->size || node->size == 0) {
        node->size = offset + bytes_written;
        
        // 我们之前约定的：把这个文件在 FAT 表中的 index 存在 node->internal 里
        uint32_t fat_idx = node->internal; 
        fat_t entry;
        
        // 读出现有目录项，修改长度和起始簇，写回磁盘
        if (lwfs_read_fat_entry_by_idx(inst, fat_idx, &entry) == 0) {
            entry.length = node->size;
            entry.cluster_start = (uint32_t)node->fs_private_data;
            lwfs_write_fat_entry_by_idx(inst, fat_idx, &entry);
        }
    }

    return bytes_written;
}
int lwfs_create(struct vfs_node* dir_node, const char* name, uint32_t flags) {
    lwfs_instance_t* inst = (lwfs_instance_t*)dir_node->fs_instance;
    
    // 1. 检查是否已存在
    vfs_node_t* check = lwfs_finddir(dir_node, name);
    if (check) {
        // 这里可以根据需要释放 check 节点
        return -1; // 已存在
    }

    // 2. 初始化 fat_t 元数据
    fat_t new_entry;
    memset(&new_entry, 0, sizeof(fat_t));
    
    // 简单处理文件名：这里需要按照你的 lwfs_format_filename 反向操作
    // 实际项目中建议写一个专门的工具函数处理文件名填充和空格补全
    strncpy(new_entry.filename, name, 28); 
    new_entry.attr = (flags & VFS_FLAG_DIR) ? FILE_DIR : FILE_PRESENT;
    new_entry.attr |= FILE_PRESENT;
    
    // 分配第一个数据簇（或者设为 EOC 等待第一次写入）
    uint32_t first_cluster = lwfs_alloc_and_link_cluster(inst, LWFAT32_EOC);
    new_entry.cluster_start = first_cluster;
    new_entry.length = 0;

    // 3. 确定父节点在 FAT 中的索引
    // 注意：根目录的 parent_idx 逻辑需要特殊处理
    uint32_t parent_fat_idx = dir_node->internal; 

    // 4. 链接到文件系统树
    uint32_t new_idx = lwfs_link_new_entry(inst, parent_fat_idx, &new_entry);
    
    return (new_idx != 0xFFFFFFFF) ? 0 : -1;
}