#include <lwos.h>
#include <video.h>
#include <string.h>
#include <pristdio.h>
#include <kb.h>
#include <pci.h>

int magic = LWOS_MAGIC;
char message[] = "ITS WORKING.\nkk";
unsigned int buffer[64];
void kernel_init() {
	writereg_video(g_640x480x2);
	memset((char*)0xa0000, 0, 43200);
	outstr("### === !!! Wellcum to LWOS Kernel !!! === ###\n");
//	InitKB();
	GetDeviceInfo(buffer,0,0,0);
	Dump256(buffer);
}
