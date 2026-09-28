#include "kbd.h"
#include "text.h"

#define KBD_DATA 0x60
#define KBD_STAT 0x64
#define KBD_CMD  0x64

// output buffer full
#define ST_OBF   0x01
// ctrl busy
#define ST_IBF   0x02
#define ST_SYS   0x04
#define ST_A2    0x08
#define ST_INH   0x10
// data from aux not kbd
#define ST_AUXB  0x20  
#define ST_TIMEO 0x40
#define ST_PERR  0x80

static int wait_write() {
    for (int i = 0; i < 100000; i++) {
        if ((inb(KBD_STAT) & ST_IBF) == 0) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

static int wait_read() {
    for (int i = 0; i < 100000; i++) {
        if (inb(KBD_STAT) & ST_OBF) {
            return 1;
        }
        io_wait();
    }
    return 0;
}

void kbd_probe() {
    BYTE s = inb(KBD_STAT);

    if (s == 0xff) {
        puts("NO 8042 RESPOND\n\r");
        return;
    }

    if  (s & ST_IBF) {
        puts("CONTROLLER NO COMMAND ACCEPTED\n\r");
        return;
    }

    if (!wait_write()) {
        puts("TIMEOUT WAIT DEVICE\n\r");
        return;
    }
    outb(KBD_CMD, 0xaa);
    if (!wait_read()) {
        puts("NO REPLY\n\r");
        return;
    }
    BYTE r = inb(KBD_DATA);
    if (r == 0x55) {
        puts("PASS\n\r");
    } else {
        puts("UNPASS\n\r");
    }

    return;
}