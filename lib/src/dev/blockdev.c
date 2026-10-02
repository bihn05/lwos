#include "dev/blockdev.h"

static PBLKDEV devices[BLKDEV_MAX_DEVICES];
static BYTE count;

void blkdev_reset(void) {
    for (BYTE i = 0; i < count; i++)
        devices[i] = 0;
    count = 0;
}

int blkdev_register(PBLKDEV d) {
    if (!d || d->version != BLKDEV_VERSION || d->size < sizeof(*d) ||
        !d->sector_size || !d->sectors || !d->read)
        return -1;
    for (BYTE i = 0; i < count; i++)
        if (devices[i] == d)
            return i;
    if (count == BLKDEV_MAX_DEVICES)
        return -1;
    devices[count] = d;
    return count++;
}

BYTE blkdev_count(void) { return count; }

PBLKDEV blkdev_get(BYTE index) {
    return index < count ? devices[index] : 0;
}
