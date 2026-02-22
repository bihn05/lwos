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
#include <mm/km.h>

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
mbr_t mbr;
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

char* hd_buffer;
void init_fs();
void load_mbr(void);
void save_mbr(void);
void fat_info(fat_t* t);
void get_fat_entry(fat_t* t, uint32_t index);
void set_fat_entry(fat_t* t, uint32_t index);
void get_cluster(uint32_t* cluster, uint32_t index);
void set_cluster(uint32_t* cluster, uint32_t index);

void init_fs() {
	hd_buffer = (char*)kmalloc(512);
	if (hd_buffer == NULL) {
		printk("Failed allocate buffer, filesystem may not work.\n");
	} else {
		printk("Sucessfully located hd buffer at 0x%08X.\n", (uint32_t)hd_buffer);
	}
}
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
		printk("");
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
	uint32_t sector_offset = index / 8;
	uint32_t entry_in_sector = index % 8;

	ata_read_sectors(0, 0, mbr.fat_offset + sector_offset, 1, hd_buffer);

	memcpy(t, hd_buffer + entry_in_sector * sizeof(fat_t), sizeof(fat_t));
}
#endif