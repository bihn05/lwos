
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#define BPS                 512
#define RSV_S               32
#define N_FATS              2
#define SPC                 1
#define FSINFO              1
#define BACKUP_BOOT_SECTOR  6
#define MIN_FAT32_CLUSTERS  65525

#define ATTR_RO             0x01
#define ATTR_ARCH           0x20
#define ATTR_VOL_ID         0x08

int main(int argn, char **argv) {
    printf("test\n");
    return 0;
}