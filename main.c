#include <lwos.h>

int magic = LWOS_MAGIC;
char message[] = "ITS WORKING.";

void kernel_init() {
	char* video = (char*)0xb8000 + 160;
	for (int i=0; i<sizeof(message); i++) {
		video[i*2] = message[i];
	}
}
