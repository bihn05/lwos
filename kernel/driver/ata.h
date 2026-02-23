// kernel/driver/ata.h

#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <kernel.h>
#include <mm/km.h>

#include <fsys/block.h>

#define ATA_PRIMARY_BASE      0x1F0
#define ATA_SECONDARY_BASE    0x170
#define ATA_PRIMARY_CTRL      0x3F6
#define ATA_SECONDARY_CTRL    0x376

// ATA Register Offsets
#define ATA_REG_DATA          0
#define ATA_REG_ERROR         1
#define ATA_REG_FEATURES      1
#define ATA_REG_SECCOUNT0     2
#define ATA_REG_LBA0          3
#define ATA_REG_LBA1          4
#define ATA_REG_LBA2          5
#define ATA_REG_HDDEVSEL      6
#define ATA_REG_COMMAND       7
#define ATA_REG_STATUS        7
#define ATA_REG_SECCOUNT1     2   // LBA48 High Byte Sector Count
#define ATA_REG_LBA3          3   // LBA48 LBA[24:31]
#define ATA_REG_LBA4          4   // LBA48 LBA[32:39]
#define ATA_REG_LBA5          5   // LBA48 LBA[40:47]

// ATA Commands
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_READ_PIO      0x20    // LBA28 Read
#define ATA_CMD_READ_PIO_EXT  0x24    // LBA48 Read
#define ATA_CMD_WRITE_PIO     0x30    // LBA28 Write
#define ATA_CMD_WRITE_PIO_EXT 0x34    // LBA48 Write
#define ATA_CMD_FLUSH_CACHE   0xE7    // Flush Cache
#define ATA_CMD_FLUSH_CACHE_E 0xEA    // Flush Extended Cache

// Status Register Bits
#define ATA_SR_BSY            0x80
#define ATA_SR_DRDY           0x40
#define ATA_SR_DF             0x20
#define ATA_SR_DSC            0x10
#define ATA_SR_DRQ            0x08
#define ATA_SR_CORR           0x04
#define ATA_SR_IDX            0x02
#define ATA_SR_ERR            0x01

// Device Types
#define ATA_DEVICE_UNKNOWN    0
#define ATA_DEVICE_PATA       1
#define ATA_DEVICE_SATA       2
#define ATA_DEVICE_PATAPI     3
#define ATA_DEVICE_SATAPI     4

// Hard Drive Information Structure
typedef struct {
    uint8_t  present;        // Device exists
    uint8_t  type;           // Device type
    uint8_t  channel;        // 0: Primary, 1: Secondary
    uint8_t  drive;          // 0: Master, 1: Slave
    uint16_t signature;      // Device signature
    uint16_t capabilities;   // Device capabilities
    uint32_t command_sets;   // Supported command sets
    uint8_t  lba48_support;  // Supports LBA48
    uint64_t size;           // Total size (in sectors)
    char     model[41];      // Model string
    char     serial[21];     // Serial number
    char     firmware[9];    // Firmware version
} ata_device_t;

void ata_init();
void ata_detect_drives();
ata_device_t* ata_get_device(uint8_t index);
uint8_t ata_get_device_count();
int ata_read_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer);
int ata_read_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer);
int ata_write_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer);
int ata_check_lba48_support(uint8_t channel, uint8_t drive);
int ata_write_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer);
int ata_flush_cache(uint8_t channel, uint8_t drive);

#define MAX_ATA_DEVICES 4

static ata_device_t ata_devices[MAX_ATA_DEVICES];
static uint8_t ata_device_count = 0;

// Wait for drive to be ready
static int ata_wait(uint16_t base, int advanced) {
    uint8_t status = 0;
    int timeout = 1000000; // Timeout counter

    // Wait for BSY bit to clear
    while (timeout-- > 0) {
        status = inb(base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) break;
        __asm volatile("nop");
    }

    if (timeout <= 0) return -1; // Timeout

    if (advanced) {
        // Check for errors
        if (status & ATA_SR_ERR) return -2;
        // Check for device fault
        if (status & ATA_SR_DF) return -3;
        // Wait for DRQ to be ready
        timeout = 1000000;
        while (timeout-- > 0) {
            status = inb(base + ATA_REG_STATUS);
            if (status & ATA_SR_DRQ) break;
        }
        if (timeout <= 0) return -4;
    }

    return 0;
}

// Select drive
static void ata_select_drive(uint16_t base, uint8_t drive) {
    outb(0x40 | (drive << 4), base + ATA_REG_HDDEVSEL); // LBA mode
    inb(base + ATA_REG_STATUS); // 400ns delay
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
}

// Check LBA48 support
int ata_check_lba48_support(uint8_t channel, uint8_t drive) {
    if (channel > 1 || drive > 1) return 0;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    ata_select_drive(base, drive);

    // Send IDENTIFY command
    outb(ATA_CMD_IDENTIFY, base + ATA_REG_COMMAND);

    if (ata_wait(base, 1) != 0) return 0;

    // Read IDENTIFY data
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base + ATA_REG_DATA);
    }

    // Check LBA48 support (Word 83 bit 10)
    return (identify_data[83] & 0x400) != 0;
}

// Read sectors (LBA48)
int ata_read_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0) return -1;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    ata_device_t* device = NULL;

    // Find device information
    for (int i = 0; i < ata_device_count; i++) {
        if (ata_devices[i].channel == channel && ata_devices[i].drive == drive) {
            device = &ata_devices[i];
            break;
        }
    }

    if (!device || !device->present) return -5;

    int use_lba48 = device->lba48_support;

    // If LBA < 2^28 and sectors <= 255, use LBA28
    if ((!use_lba48) && (lba < 0x10000000) && (sectors <= 255)) {
        return ata_read_sectors_lba28(channel, drive, (uint32_t)lba, (uint8_t)sectors, buffer);
    }

    if (!use_lba48) {
        printk("LBA48 not supported but required for LBA %llu\n", lba);
        return -6;
    }

    // LBA48 Read
    ata_select_drive(base, drive);

    // Set LBA48 parameters
    outb((sectors >> 8) & 0xFF, base + ATA_REG_SECCOUNT1);   // Sector count high byte
    outb((lba >> 24) & 0xFF, base + ATA_REG_LBA3);          // LBA[24:31]
    outb((lba >> 32) & 0xFF, base + ATA_REG_LBA4);          // LBA[32:39]
    outb((lba >> 40) & 0xFF, base + ATA_REG_LBA5);          // LBA[40:47]

    outb(sectors & 0xFF, base + ATA_REG_SECCOUNT0);         // Sector count low byte
    outb(lba & 0xFF, base + ATA_REG_LBA0);                  // LBA[0:7]
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);           // LBA[8:15]
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);          // LBA[16:23]

    // Send LBA48 read command
    outb(ATA_CMD_READ_PIO_EXT, base + ATA_REG_COMMAND);

    // Wait for ready
    int a = ata_wait(base, 1);
    if (a != 0) return -2;

    // Read data
    uint16_t* target = (uint16_t*)buffer;
    uint32_t total_words = sectors * 256; // 256 words per sector

    for (uint32_t i = 0; i < total_words; i++) {
        // Check status periodically (every 256 words)
        if ((i % 256) == 0 && i > 0) {
            if (ata_wait(base, 1) != 0) return -3;
        }
        target[i] = inw(base + ATA_REG_DATA);
    }

    return 0;
}

// Read sectors LBA28 (Fallback)
int ata_read_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0 || sectors > 255) return -1;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    // Select drive and LBA mode
    outb(0xE0 | (drive << 4) | ((lba >> 24) & 0x0F), base + ATA_REG_HDDEVSEL);

    // Set parameters
    outb(sectors, base + ATA_REG_SECCOUNT0);
    outb(lba & 0xFF, base + ATA_REG_LBA0);
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);

    // Send read command
    outb(ATA_CMD_READ_PIO, base + ATA_REG_COMMAND);

    // Wait for ready
    if (ata_wait(base, 1) != 0) return -2;

    // Read data
    uint16_t* target = (uint16_t*)buffer;
    for (int s = 0; s < sectors; s++) {
        // Wait for current sector to be ready
        if (ata_wait(base, 1) != 0) return -3;

        // Read 256 words (512 bytes)
        for (int i = 0; i < 256; i++) {
            target[i] = inw(base + ATA_REG_DATA);
        }
        target += 256;
    }

    return 0;
}

// Identify ATA device (Updated to detect LBA48 support)
static int ata_identify(uint16_t base, uint8_t drive, ata_device_t* device) {
    ata_select_drive(base, drive);

    // Send IDENTIFY command
    outb(ATA_CMD_IDENTIFY, base + ATA_REG_COMMAND);

    // Check if device exists
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0) {
        return -1; // Device not found
    }

    // Wait for command completion
    if (ata_wait(base, 1) != 0) {
        return -2; // Command failed
    }

    // Read IDENTIFY data
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base + ATA_REG_DATA);
    }

    // Parse device information
    device->present = 1;
    device->channel = (base == ATA_PRIMARY_BASE) ? 0 : 1;
    device->drive = drive;
    device->signature = identify_data[0];

    // Check LBA48 support
    device->lba48_support = (identify_data[83] & 0x400) != 0;

    // Parse model, serial, and firmware version
    for (int i = 0; i < 20; i++) {
        device->model[i * 2] = (identify_data[27 + i] >> 8) & 0xFF;
        device->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }
    device->model[40] = '\0';

    for (int i = 0; i < 10; i++) {
        device->serial[i * 2] = (identify_data[10 + i] >> 8) & 0xFF;
        device->serial[i * 2 + 1] = identify_data[10 + i] & 0xFF;
    }
    device->serial[20] = '\0';

    for (int i = 0; i < 4; i++) {
        device->firmware[i * 2] = (identify_data[23 + i] >> 8) & 0xFF;
        device->firmware[i * 2 + 1] = identify_data[23 + i] & 0xFF;
    }
    device->firmware[8] = '\0';

    // Determine device type
    if (identify_data[0] == 0x848A || identify_data[0] == 0x844A) {
        device->type = ATA_DEVICE_PATAPI;
    }
    else {
        device->type = ATA_DEVICE_PATA;
    }

    // Get capacity (Prefer LBA48)
    if (device->lba48_support) {
        device->size = *((uint64_t*)&identify_data[100]);
    }
    else {
        // Use LBA28
        device->size = *((uint32_t*)&identify_data[60]);
    }

    return 0;
}

// LBA28 Write Sectors Function
int ata_write_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0 || sectors > 255) {
        printk("Invalid write parameters: channel=%d, drive=%d, sectors=%d\n", channel, drive, sectors);
        return -1;
    }

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    printk("Writing LBA28: channel=%d, drive=%d, lba=%u, sectors=%d\n", channel, drive, lba, sectors);

    // Select drive and LBA mode
    uint8_t dev_sel = 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F);
    outb(dev_sel, base + ATA_REG_HDDEVSEL);

    // Short delay
    for (int i = 0; i < 1000; i++) asm volatile ("nop");

    // Check if device is ready
    if (ata_wait(base, 0) != 0) {
        printk("Device not ready before write command\n");
        return -2;
    }

    // Set parameters
    outb(sectors, base + ATA_REG_SECCOUNT0);
    outb(lba & 0xFF, base + ATA_REG_LBA0);
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);

    // Send write command
    outb(ATA_CMD_WRITE_PIO, base + ATA_REG_COMMAND);

    // Wait for device to be ready to receive data
    int wait_result = ata_wait(base, 1);
    if (wait_result != 0) {
        printk("ATA Wait failed before data transfer: %d\n", wait_result);
        return wait_result;
    }

    // Write data
    uint16_t* source = (uint16_t*)buffer;
    for (int s = 0; s < sectors; s++) {
        // Wait for device to be ready for current sector
        if (ata_wait(base, 1) != 0) {
            printk("Failed waiting for sector %d ready for write\n", s);
            return -3;
        }

        // Write 256 words (512 bytes)
        for (int i = 0; i < 256; i++) {
            outw(source[i], base + ATA_REG_DATA);
            __asm volatile ("nop");
        }
        source += 256;

        //printk("Sector %d written successfully\n", s);

        // Wait for device to finish processing current sector
        if (ata_wait(base, 0) != 0) {
            printk("Device busy after writing sector %d\n", s);
            return -4;
        }
    }

    printk("All sectors written successfully\n");
    return 0;
}

// Write sectors (LBA48)
int ata_write_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0)return -1;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    ata_device_t* device = NULL;

    // Find device information
    for (int i = 0; i < ata_device_count; i++) {
        if (ata_devices[i].channel == channel && ata_devices[i].drive == drive) {
            device = &ata_devices[i];
            break;
        }
    }

    if (!device || !device->present)return -5;

    int use_lba48 = device->lba48_support;

    // if lba < 2^28 and sectors <= 255, then use lba28
    if ((!use_lba48) && (lba < 0x10000000) && (sectors <= 255)) {
        return ata_write_sectors_lba28(channel, drive, (uint32_t)lba, (uint8_t)sectors, buffer);
    }

    if (!use_lba48) {
        printk("LBA48 not supported but required for LBA %llu\n", lba);
        return -6;
    }

    // LBA48 Write
    ata_select_drive(base, drive);

    // Set LBA48 param
    outb((sectors >> 8) & 0xFF, base + ATA_REG_SECCOUNT1);  // Sector count high
    outb((lba >> 24) & 0xFF, base + ATA_REG_LBA3);          // LBA[24:31]
    outb((lba >> 32) & 0xFF, base + ATA_REG_LBA4);          // LBA[32:39]
    outb((lba >> 40) & 0xFF, base + ATA_REG_LBA5);          // LBA[40:47]

    outb(sectors & 0xFF, base + ATA_REG_SECCOUNT0);         // Sector count low
    outb(lba & 0xFF, base + ATA_REG_LBA0);                  // LBA[0:7]
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);           // LBA[8:15]
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);          // LBA[16:23]

    // Send LBA48 write command
    outb(ATA_CMD_WRITE_PIO_EXT, base + ATA_REG_COMMAND);

    // Wait for ready
    int a = ata_wait(base, 1);
    if (a != 0)return -2;

    // Write data
    uint16_t* source = (uint16_t*)buffer;
    for (int s = 0; s < sectors; s++) {
        // Wait for device to be ready for current sector
        if (ata_wait(base, 1) != 0) {
            printk("Failed waiting for sector %d read for write\n", s);
            return -3;
        }

        // Write 256 words (512 bytes)
        for (int i = 0; i < 256; i++) {
            outw(source[i], base + ATA_REG_DATA);
            __asm volatile ("nop");
        }
        source += 256;

        // Wait for device to finish processing current sector
        if (ata_wait(base, 0) != 0) {
            printk("Device busy after writing sector %d", s);
            return -4;
        }
    }

    printk("All sectors written successfully\n");
    return 0;
}

// Initialize ATA driver
void ata_init() {
    memset(ata_devices, 0, sizeof(ata_devices));
    ata_device_count = 0;

    // Disable interrupts
    outb(0x02, ATA_PRIMARY_CTRL);
    outb(0x02, ATA_SECONDARY_CTRL);
}

// Detect all ATA devices
void ata_detect_drives() {
    printk("Detecting ATA devices...\n");

    uint16_t bases[] = { ATA_PRIMARY_BASE, ATA_SECONDARY_BASE };
    const char* channel_names[] = { "Primary", "Secondary" };
    const char* drive_names[] = { "Master", "Slave" };
    const char* lba_mode[] = { "LBA24","LBA48" };

    for (int channel = 0; channel < 2; channel++) {
        uint16_t base = bases[channel];

        for (int drive = 0; drive < 2; drive++) {
            if (ata_device_count >= MAX_ATA_DEVICES) break;

            // Check if device exists
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
                // Device exists, attempt to identify
                ata_device_t* device = &ata_devices[ata_device_count];
                if (ata_identify(base, drive, device) == 0) {
                    printk("Found %s Channel %s: %s\n --> (%s), %d sectors\n",
                        channel_names[channel], drive_names[drive],
                        device->model, lba_mode[device->lba48_support], device->size);
                    ata_device_count++;
                }
            }
        }
    }

    printk("Total ATA devices found: %d\n", ata_device_count);
}

// Get device information
ata_device_t* ata_get_device(uint8_t index) {
    if (index >= ata_device_count) return NULL;
    return &ata_devices[index];
}

uint8_t ata_get_device_count() {
    return ata_device_count;
}

static int ata_block_read(block_dev_t* dev, uint64_t lba, uint32_t count, void* buffer) {
    ata_device_t* ata_dev = (ata_device_t*)dev->private_data;
    return ata_read_sectors(ata_dev->channel, ata_dev->drive, lba, (uint16_t)count, buffer);
}
static int ata_block_write(block_dev_t* dev, uint64_t lba, uint32_t count, void* buffer) {
    ata_device_t* ata_dev = (ata_device_t*)dev->private_data;
    return ata_write_sectors(ata_dev->channel, ata_dev->drive, lba, (uint16_t)count, buffer);
}
static block_dev_ops_t ata_block_ops = {
    .read = ata_block_read,
    .write = ata_block_write,
    .get_info = NULL
};
block_dev_t* ata_get_block_device_ptr(uint8_t index) {
    ata_device_t* ata_dev = ata_get_device(index);
    if (!ata_dev) {
        printk("Invalid ATA device index: %d\n", index);
        return NULL;
    }
    printk("Creating block device for ATA device %d\n", index);
    block_dev_t* bdev = (block_dev_t*)kmalloc(sizeof(block_dev_t));
    strncpy(bdev->dev_name, ata_dev->model, 31);
    bdev->sector_size = 512;
    bdev->total_sectors = ata_dev->size;
    bdev->ops = &ata_block_ops;
    bdev->private_data = (void*)ata_dev;
    
    return bdev;
}

#endif