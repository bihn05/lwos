#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <kernel.h>

#define ATA_PRIMARY_BASE      0x1F0
#define ATA_SECONDARY_BASE    0x170
#define ATA_PRIMARY_CTRL      0x3F6
#define ATA_SECONDARY_CTRL    0x376

// ATA寄存器偏移
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
#define ATA_REG_SECCOUNT1     2   // LBA48 高字节扇区数
#define ATA_REG_LBA3          3   // LBA48 LBA[24:31]
#define ATA_REG_LBA4          4  // LBA48 LBA[32:39]
#define ATA_REG_LBA5          5  // LBA48 LBA[40:47]

// ATA命令
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_READ_PIO      0x20    // LBA28 读取
#define ATA_CMD_READ_PIO_EXT  0x24    // LBA48 读取
#define ATA_CMD_WRITE_PIO     0x30    // LBA28 写入
#define ATA_CMD_WRITE_PIO_EXT 0x34    // LBA48 写入
#define ATA_CMD_FLUSH_CACHE   0xE7    // 刷新缓存
#define ATA_CMD_FLUSH_CACHE_E 0xEA    // 刷新扩展缓存

// 状态寄存器位
#define ATA_SR_BSY            0x80
#define ATA_SR_DRDY           0x40
#define ATA_SR_DF             0x20
#define ATA_SR_DSC            0x10
#define ATA_SR_DRQ            0x08
#define ATA_SR_CORR           0x04
#define ATA_SR_IDX            0x02
#define ATA_SR_ERR            0x01

// 设备类型
#define ATA_DEVICE_UNKNOWN    0
#define ATA_DEVICE_PATA       1
#define ATA_DEVICE_SATA       2
#define ATA_DEVICE_PATAPI     3
#define ATA_DEVICE_SATAPI     4

// 硬盘信息结构
typedef struct {
    uint8_t  present;        // 设备是否存在
    uint8_t  type;           // 设备类型
    uint8_t  channel;        // 0: Primary, 1: Secondary
    uint8_t  drive;          // 0: Master, 1: Slave
    uint16_t signature;      // 设备签名
    uint16_t capabilities;   // 设备能力
    uint32_t command_sets;   // 支持的指令集
    uint8_t  lba48_support;  // 是否支持LBA48
    uint64_t size;           // 总大小（扇区数）
    char     model[41];      // 型号字符串
    char     serial[21];     // 序列号
    char     firmware[9];    // 固件版本
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

// 等待硬盘就绪
static int ata_wait(uint16_t base, int advanced) {
    uint8_t status = 0;
    int timeout = 1000000; // 超时计数器

    // 等待BSY位清除
    while (timeout-- > 0) {
        status = inb(base + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) break;
        __asm volatile("nop");
    }

    if (timeout <= 0) return -1; // 超时

    if (advanced) {
        // 检查错误
        if (status & ATA_SR_ERR) return -2;
        // 检查设备故障
        if (status & ATA_SR_DF) return -3;
        // 等待DRQ就绪
        timeout = 1000000;
        while (timeout-- > 0) {
            status = inb(base + ATA_REG_STATUS);
            if (status & ATA_SR_DRQ) break;
        }
        if (timeout <= 0) return -4;
    }

    return 0;
}

// 选择硬盘
static void ata_select_drive(uint16_t base, uint8_t drive) {
    outb(0x40 | (drive << 4), base + ATA_REG_HDDEVSEL); // LBA模式
    inb(base + ATA_REG_STATUS); // 400ns延迟
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
    inb(base + ATA_REG_STATUS);
}

// 检查LBA48支持
int ata_check_lba48_support(uint8_t channel, uint8_t drive) {
    if (channel > 1 || drive > 1) return 0;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    ata_select_drive(base, drive);

    // 发送IDENTIFY命令
    outb(ATA_CMD_IDENTIFY, base + ATA_REG_COMMAND);

    if (ata_wait(base, 1) != 0) return 0;

    // 读取IDENTIFY数据
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base + ATA_REG_DATA);
    }

    // 检查LBA48支持 (字83的位10)
    return (identify_data[83] & 0x400) != 0;
}

// LBA48读取扇区
int ata_read_sectors(uint8_t channel, uint8_t drive, uint64_t lba, uint16_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0) return -1;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;
    ata_device_t* device = NULL;

    // 查找设备信息
    for (int i = 0; i < ata_device_count; i++) {
        if (ata_devices[i].channel == channel && ata_devices[i].drive == drive) {
            device = &ata_devices[i];
            break;
        }
    }

    if (!device || !device->present) return -5;

    int use_lba48 = device->lba48_support;

    // 如果LBA地址小于2^28且扇区数<=255，可以使用LBA28
    if ((!use_lba48) && (lba < 0x10000000) && (sectors <= 255)) {
        return ata_read_sectors_lba28(channel, drive, (uint32_t)lba, (uint8_t)sectors, buffer);
    }

    if (!use_lba48) {
        printk("LBA48 not supported but required for LBA %llu\n", lba);
        return -6;
    }

    // LBA48 读取
    ata_select_drive(base, drive);

    // 设置LBA48参数
    outb((sectors >> 8) & 0xFF, base + ATA_REG_SECCOUNT1);   // 扇区数高字节
    outb((lba >> 24) & 0xFF, base + ATA_REG_LBA3);          // LBA[24:31]
    outb((lba >> 32) & 0xFF, base + ATA_REG_LBA4);          // LBA[32:39]
    outb((lba >> 40) & 0xFF, base + ATA_REG_LBA5);          // LBA[40:47]

    outb(sectors & 0xFF, base + ATA_REG_SECCOUNT0);         // 扇区数低字节
    outb(lba & 0xFF, base + ATA_REG_LBA0);                  // LBA[0:7]
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);           // LBA[8:15]
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);          // LBA[16:23]

    // 发送LBA48读取命令
    outb(ATA_CMD_READ_PIO_EXT, base + ATA_REG_COMMAND);

    // 等待就绪
    int a = ata_wait(base, 1);
    if (a != 0) return -2;

    // 读取数据
    uint16_t* target = (uint16_t*)buffer;
    uint32_t total_words = sectors * 256; // 每个扇区256字

    for (uint32_t i = 0; i < total_words; i++) {
        // 定期检查状态（每256字检查一次）
        if ((i % 256) == 0 && i > 0) {
            if (ata_wait(base, 1) != 0) return -3;
        }
        target[i] = inw(base + ATA_REG_DATA);
    }

    return 0;
}

// LBA28读取扇区（备用）
int ata_read_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0 || sectors > 255) return -1;

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    // 选择驱动器和LBA模式
    outb(0xE0 | (drive << 4) | ((lba >> 24) & 0x0F), base + ATA_REG_HDDEVSEL);

    // 设置参数
    outb(sectors, base + ATA_REG_SECCOUNT0);
    outb(lba & 0xFF, base + ATA_REG_LBA0);
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);

    // 发送读取命令
    outb(ATA_CMD_READ_PIO, base + ATA_REG_COMMAND);

    // 等待就绪
    if (ata_wait(base, 1) != 0) return -2;

    // 读取数据
    uint16_t* target = (uint16_t*)buffer;
    for (int s = 0; s < sectors; s++) {
        // 等待当前扇区就绪
        if (ata_wait(base, 1) != 0) return -3;

        // 读取256个字（512字节）
        for (int i = 0; i < 256; i++) {
            target[i] = inw(base + ATA_REG_DATA);
        }
        target += 256;
    }

    return 0;
}

// 识别ATA设备（更新以检测LBA48支持）
static int ata_identify(uint16_t base, uint8_t drive, ata_device_t* device) {
    ata_select_drive(base, drive);

    // 发送IDENTIFY命令
    outb(ATA_CMD_IDENTIFY, base + ATA_REG_COMMAND);

    // 检查设备是否存在
    uint8_t status = inb(base + ATA_REG_STATUS);
    if (status == 0) {
        return -1; // 设备不存在
    }

    // 等待命令完成
    if (ata_wait(base, 1) != 0) {
        return -2; // 命令执行失败
    }

    // 读取IDENTIFY数据
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(base + ATA_REG_DATA);
    }

    // 解析设备信息
    device->present = 1;
    device->channel = (base == ATA_PRIMARY_BASE) ? 0 : 1;
    device->drive = drive;
    device->signature = identify_data[0];

    // 检查LBA48支持
    device->lba48_support = (identify_data[83] & 0x400) != 0;

    // 解析型号、序列号、固件版本
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

    // 确定设备类型
    if (identify_data[0] == 0x848A || identify_data[0] == 0x844A) {
        device->type = ATA_DEVICE_PATAPI;
    }
    else {
        device->type = ATA_DEVICE_PATA;
    }

    // 获取容量（优先使用LBA48）
    if (device->lba48_support) {
        device->size = *((uint64_t*)&identify_data[100]);
    }
    else {
        // 使用LBA28
        device->size = *((uint32_t*)&identify_data[60]);
    }

    return 0;
}

// LBA28 写扇区函数
int ata_write_sectors_lba28(uint8_t channel, uint8_t drive, uint32_t lba, uint8_t sectors, void* buffer) {
    if (channel > 1 || drive > 1 || sectors == 0 || sectors > 255) {
        printk("Invalid write parameters: channel=%d, drive=%d, sectors=%d\n", channel, drive, sectors);
        return -1;
    }

    uint16_t base = (channel == 0) ? ATA_PRIMARY_BASE : ATA_SECONDARY_BASE;

    printk("Writing LBA28: channel=%d, drive=%d, lba=%u, sectors=%d\n", channel, drive, lba, sectors);

    // 选择驱动器和LBA模式
    uint8_t dev_sel = 0xE0 | (drive << 4) | ((lba >> 24) & 0x0F);
    outb(dev_sel, base + ATA_REG_HDDEVSEL);

    // 短暂延迟
    for (int i = 0; i < 1000; i++) asm volatile ("nop");

    // 检查设备是否就绪
    if (ata_wait(base, 0) != 0) {
        printk("Device not ready before write command\n");
        return -2;
    }

    // 设置参数
    outb(sectors, base + ATA_REG_SECCOUNT0);
    outb(lba & 0xFF, base + ATA_REG_LBA0);
    outb((lba >> 8) & 0xFF, base + ATA_REG_LBA1);
    outb((lba >> 16) & 0xFF, base + ATA_REG_LBA2);

    // 发送写入命令
    outb(ATA_CMD_WRITE_PIO, base + ATA_REG_COMMAND);

    // 等待设备准备接收数据
    int wait_result = ata_wait(base, 1);
    if (wait_result != 0) {
        printk("ATA Wait failed before data transfer: %d\n", wait_result);
        return wait_result;
    }

    // 写入数据
    uint16_t* source = (uint16_t*)buffer;
    for (int s = 0; s < sectors; s++) {
        // 等待设备准备好接收当前扇区数据
        if (ata_wait(base, 1) != 0) {
            printk("Failed waiting for sector %d ready for write\n", s);
            return -3;
        }

        // 写入256个字（512字节）
        for (int i = 0; i < 256; i++) {
            outw(source[i], base + ATA_REG_DATA);
            __asm volatile ("nop");
        }
        source += 256;

        //printk("Sector %d written successfully\n", s);

        // 等待设备处理完当前扇区
        if (ata_wait(base, 0) != 0) {
            printk("Device busy after writing sector %d\n", s);
            return -4;
        }
    }

    printk("All sectors written successfully\n");
    return 0;
}

// 初始化ATA驱动
void ata_init() {
    memset(ata_devices, 0, sizeof(ata_devices));
    ata_device_count = 0;

    // 禁用中断
    outb(0x02, ATA_PRIMARY_CTRL);
    outb(0x02, ATA_SECONDARY_CTRL);
}

// 检测所有ATA设备
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

            // 检查设备是否存在
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
                // 设备存在，尝试识别
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

// 获取设备信息
ata_device_t* ata_get_device(uint8_t index) {
    if (index >= ata_device_count) return NULL;
    return &ata_devices[index];
}

uint8_t ata_get_device_count() {
    return ata_device_count;
}

#endif