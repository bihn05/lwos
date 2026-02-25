// kernel/driver/ata.h

#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <printk.h>
#include <mm/km.h>
#include <driver/block.h>

// #include <fsys/block.h>

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

#define MAX_ATA_DEVICES 4
extern ata_device_t ata_devices[MAX_ATA_DEVICES];
extern uint8_t ata_device_count;

static int ata_wait(uint16_t base, int advanced);
static void ata_select_drive(uint16_t base, uint8_t drive);
int ata_check_lba48_support(uint8_t channel, uint8_t drive);
int ata_read_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer);
int ata_read_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer);
static int ata_identify(uint16_t base, uint8_t drive, ata_device_t* device);
int ata_write_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer);
int ata_write_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer);
void ata_init();
void ata_detect_drives();
ata_device_t* ata_get_device(uint8_t index);
uint8_t ata_get_device_count();
static int ata_block_read(block_dev_t* dev, uint64_t lba, uint32_t count, void* buffer);
static int ata_block_write(block_dev_t* dev, uint64_t lba, uint32_t count, void* buffer);
block_dev_t* ata_get_block_device_ptr(uint8_t index);

#endif