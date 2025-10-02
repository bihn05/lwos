#ifndef _HARD_DRIVE_H
#define _HARD_DRIVE_H

#include <stdint.h>
#include <string.h>
#include <driver/io.h>
#include <kernel.h>

#define ATA_PRI_BASE 0x1F0
#define ATA_SEC_BASE 0x170
#define ATA_PRI_CTRL 0x3F6
#define ATA_SEC_CTRL 0x376

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0      0x03
#define ATA_REG_LBA1      0x04
#define ATA_REG_LBA2      0x05
#define ATA_REG_HDDEVSEL  0x06
#define ATA_REG_COMMAND   0x07
#define ATA_REG_STATUS    0x07

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_WRITE_PIO         0x30

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

#define ATA_DEVICE_UNKNOWN 0x00
#define ATA_DEVICE_PATA  0x01
#define ATA_DEVICE_SATA  0x02
#define ATA_DEVICE_PATAPI 0x03
#define ATA_DEVICE_SATAPI 0x04

typedef struct {
	uint8_t present;
	uint8_t type;
	uint8_t channel;
	uint8_t drive;
	uint16_t signature;
	uint16_t capabilities;
	uint32_t command_sets;
	uint32_t size; // Size in sectors
	char model[41];
	char serial[21];
	char firmware[9];
} ata_dev_t;
#define MAX_ATA_DEVICES 4
static ata_dev_t ata_device[MAX_ATA_DEVICES]; // 2 channels, 2 drives each
static uint8_t ata_dev_count = 0;

void ata_init();
void ata_detect();
ata_dev_t* ata_get_dev(uint8_t index);
uint8_t ata_get_dev_count();
int ata_read(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer);
static int ata_wait(uint16_t base, int advanced_check) {
	uint8_t status = 0;
	int timeout = 1000000;

	while (timeout-- > 0) {
		status = inb(base + ATA_REG_STATUS);
		if (!(status & ATA_SR_BSY))break;
	}

	if (timeout <= 0)return -1; // Timeout

	if (advanced_check) {
		if (status & ATA_SR_ERR)return -2; // Error
		if (status & ATA_SR_DF)return -3; // Device Fault
		timeout = 1000000;
		while (timeout-- > 0) {
			status = inb(base + ATA_REG_STATUS);
			if (status & ATA_SR_DRQ)break;
		}
		if (timeout <= 0)return -4; // Timeout
	}

	return 0; // No Error
}
static void ata_select_drive(uint16_t base, uint8_t drive) {
	outb(0xA0 | (drive << 4), base + ATA_REG_HDDEVSEL);
	inb(base + ATA_REG_STATUS); // Delay
	inb(base + ATA_REG_STATUS); // Delay
	inb(base + ATA_REG_STATUS); // Delay
	inb(base + ATA_REG_STATUS); // Delay
}
static int ata_identify(uint16_t base, uint8_t drive, ata_dev_t* dev) {
	ata_select_drive(base, drive);
	outb(0, base + ATA_REG_SECCOUNT0);
	outb(0, base + ATA_REG_LBA0);
	outb(0, base + ATA_REG_LBA1);
	outb(0, base + ATA_REG_LBA2);
	outb(0xEC, base + ATA_REG_COMMAND); // IDENTIFY command

	uint8_t status = inb(base + ATA_REG_STATUS);
	if (status == 0)return -1; // No device

	if (ata_wait(base, 1) != 0)return -2; // Error or timeout

	uint16_t identify_buffer[256];
	for (int i = 0; i < 256; i++) {
		identify_buffer[i] = inw(base + ATA_REG_DATA);
	}

	dev->present = 1;
	dev->channel = (base == ATA_PRI_BASE) ? 0 : 1;
	dev->drive = drive;
	dev->signature = identify_buffer[0];

	for (int i = 0; i < 20; i += 2) {
		dev->model[i] = identify_buffer[27 + i / 2] >> 8;
		dev->model[i + 1] = identify_buffer[27 + i / 2] & 0xFF;
	}
	dev->model[40] = '\0';

	for (int i = 0; i < 10; i += 2) {
		dev->serial[i] = identify_buffer[10 + i / 2] >> 8;
		dev->serial[i + 1] = identify_buffer[10 + i / 2] & 0xFF;
	}
	dev->serial[20] = '\0';

	for (int i = 0; i < 4; i++) {
		dev->firmware[i] = identify_buffer[23 + i / 2] >> 8;
		dev->firmware[i + 1] = identify_buffer[23 + i / 2] & 0xFF;
	}
	dev->firmware[8] = '\0';

	if (identify_buffer[0] == 0x848a || identify_buffer[0] == 0x844a) {
		dev->type = ATA_DEVICE_PATAPI;
	}
	else {
		dev->type = ATA_DEVICE_PATA;
	}

	if (identify_buffer[83] & 0x400) {
		dev->size = *((uint64_t*)&identify_buffer[100]);
	}
	else {
		dev->size = *((uint32_t*)&identify_buffer[60]);
	}

	return 0; // No Error
}
void ata_init() {
	memset(ata_device, 0, sizeof(ata_device));
	ata_dev_count = 0;

	outb(0x02, ATA_PRI_CTRL);
	outb(0x02, ATA_SEC_CTRL);
}
void ata_detect() {
	printk(" - Detecting ATA devices.\n");
	uint16_t bases[] = {ATA_PRI_BASE, ATA_SEC_BASE};
	const char* channel_names[] = {"Primary", "Secondy"};
	const char* drive_names[] = {"Master", "Slave"};

	for (int channel = 0; channel < 2; channel++) {
		uint16_t base = bases[channel];

		for (int drive = 0; drive < 2; drive++) {
			if (ata_dev_count >= MAX_ATA_DEVICES)break;

			ata_select_drive(base, drive);
			outb(0x55, base + ATA_REG_SECCOUNT0);
			outb(0xAA, base + ATA_REG_LBA0);
			outb(0xAA, base + ATA_REG_SECCOUNT0);
			outb(0x55, base + ATA_REG_LBA0);
			outb(0x55, base + ATA_REG_SECCOUNT0);
			outb(0xAA, base + ATA_REG_LBA0);

			uint8_t sec = inb(base + ATA_REG_SECCOUNT0);
			uint8_t lba = inb(base + ATA_REG_LBA0);

			if (sec == 0x55 && lba == 0xAA) {
				ata_dev_t* device = &ata_device[ata_dev_count];
				if (ata_identify(base, drive, device) == 0) {
					printk(" - Found %s channel %s: %s, %d sectors\n",
						channel_names[channel], drive_names[drive],
						device->model, device->size);
					ata_dev_count++;
				}
			}
		}
	}

	printk(" - Total ATA devices found: %d\n", ata_dev_count);
}
int ata_read(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer) {
	if (channel > 1 || drive > 1 || sectors == 0 || sectors > 255)return -1;

	uint16_t base = (channel == 0) ? ATA_PRI_BASE : ATA_SEC_BASE;

	outb(0xE0 | (drive << 4) | ((lba >> 24) & 0x0F), base + ATA_REG_HDDEVSEL);

	outb(sectors, base + ATA_REG_SECCOUNT0);
	outb(lba & 0xFF, base + ATA_REG_LBA0);
	outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);
	outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);

	outb(ATA_CMD_READ_PIO, base + ATA_REG_COMMAND);

	if (ata_wait(base, 1) != 0)return -2;

	uint16_t* target = (uint16_t*)buffer;
	for (int s = 0; s < sectors; s++) {
		if (ata_wait(base, 1) != 0)return -3;

		for (int i = 0; i < 256; i++) {
			target[i] = inw(base + ATA_REG_DATA);
		}
		target += 256;
	}

	return 0;
}
ata_dev_t* ata_get_dev(uint8_t index) {
	if (index >= ata_dev_count)return NULL;
	return &ata_device[index];
}
uint8_t ata_get_dev_count() {
	return ata_dev_count;
}

#endif