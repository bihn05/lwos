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
#pragma pack(push, 8)
typedef struct {
	uint8_t jmp_code[3];
	char filesys_name[8];
	char reserved[5];
	uint64_t partition_offset; // 分区起始LBA (sct)
	uint64_t partition_length; // 分区扇区数 (sct)
	uint32_t fat_offset; // FAT相对偏移量 (sct)
	uint32_t fat_length; // FAT扇区数 (sct)
	uint32_t cluster_offset; // 簇堆相对偏移量 (sct)
	uint32_t cluster_count; // 簇数 (cst)
	uint32_t root_cluster; // 根目录起始簇号 (cst)
	uint32_t volume_serial_number; // 卷序列号
	uint16_t fs_version; // 文件系统版本
	uint8_t sector_shift; // exp(2, shift)
	uint8_t clustor_shift; // exp(2, shift)
	uint8_t flags;
	uint8_t drive_select;
	uint8_t used_percent;
	char reserved2;
} mbr_t;
#pragma pack(pop)
mbr_t mbr;
#pragma pack(push, 8)
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

char hd_buffer[512];
void load_mbr(void);
void save_mbr(void);
void fat_info(fat_t* t);
void get_fat_entry(fat_t* t, uint32_t index);
void set_fat_entry(fat_t* t, uint32_t index);
void get_cluster(uint32_t* cluster, uint32_t index);
void set_cluster(uint32_t* cluster, uint32_t index);

void load_mbr(void) {
	int a= ata_read_sectors(0, 0, 0, 1, &mbr);
	printk("returned %d\n", a);
	printk("filesystem name:      %s\n", mbr.filesys_name);
	printk("partition start lba:  %d\n", mbr.partition_offset);
	printk("partition sectors:    %d\n", mbr.partition_length);
	printk("fat r-offset:         %d\n", mbr.fat_offset);
	printk("fat sectors:          %d\n", mbr.fat_length);
	printk("cluster heap r-offset:%d\n", mbr.cluster_offset);
	printk("cluster counts:       %d\n", mbr.cluster_count);
	printk("root start cluster:   %d\n", mbr.root_cluster);
	printk("sectors per cluster:  %d(%dKB)\n",
		(1 << mbr.clustor_shift), (512 << mbr.clustor_shift) / 1024);
}
void fat_info(fat_t* t) {
//	if ((t->attr & FILE_PRESENT) == 0) {
//		printk("file not exist\n");
//		return;
//	}
	if (t->attr & FILE_DIR) {
		printk("** directory **\n");
	}
	else {
		printk("file name:      %s\n", t->filename);
		printk("extend name:    %s\n", t->extname);
		printk("file length:    %d\n", t->length);
		printk("start cluster:  %d\n", t->cluster_start);
		printk("flags:        0x%02X\n", t->attr);
	}
}
void get_fat_entry(fat_t* t, uint32_t index) {
	if (index > mbr.fat_length * 8) {
		printk("invaild index\n");
		return;
	}
	ata_read_sectors(0, 0, mbr.fat_offset + index / 8, 1, hd_buffer);
	memcpy((char*)t + 8 * (index % 8), hd_buffer, 64);
}
#endif
