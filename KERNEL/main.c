#include <lwos.h>
#include <video.h>

int magic = LWOS_MAGIC;
char message[] = "ITS WORKING.";

void kernel_init() {
	writereg_video(g_640x480x2);
	char* video = (char*)0xa0000;
	for (int i=0;i<0x10000;i++) {
		video[i] = 0x55;
	}
}
