#include <stdint.h>

void memset(uint8_t* dist, uint8_t val, int32_t count) {
	for (int i=0;i<count;i++) {
		*dist = val;
		dist++;
	}
}
void memcpy(uint8_t* dist, uint8_t* sour, int32_t count) {
	for (int i=0;i<count;i++) {
		*dist = *sour;
		dist++;
		sour++;
	}
}
