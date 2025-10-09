#ifndef _BITMAP_H
#define _BITMAP_H

#include <stdint.h>
#include <string.h>

typedef struct {
	uint8_t* bits;		// 位图缓冲区
	uint32_t length;	// 位图长度
//	uint32_t offset;	// 位图开始的偏移
} bbitmap_t;

int bbitmap_test(bbitmap_t* map, int index);
void bbitmap_set(bbitmap_t* map, int index, int value);
int bbitmap_find_continuous(bbitmap_t* map, int size);

int bbitmap_test(bbitmap_t* map, int index) {
	uint8_t tmp;
	tmp = map->bits[index / 4] >> ((index % 4) * 2);  // 每个索引占2位
	tmp &= 3;
	return tmp;
}
void bbitmap_set(bbitmap_t* map, int index, int value) {
	int byte_index = index / 4;
	int bit_offset = (index % 4) * 2;
	uint8_t mask = ~(3 << bit_offset);
	uint8_t new_value = (value & 3) << bit_offset;

	map->bits[byte_index] = (map->bits[byte_index] & mask) | new_value;
}
int bbitmap_find_continuous(bbitmap_t* map, int size) {
    if (map == NULL || size <= 0 || size > map->length) {
        return -1;
    }

    int consecutive_zeros = 0;
    int start_pos = -1;

    for (int i = 0; i < map->length; i++) {
        if (bbitmap_test(map, i) == 0) {
            if (consecutive_zeros == 0) {
                start_pos = i;  // 记录可能的起始位置
            }
            consecutive_zeros++;

            // 找到足够大的连续空间
            if (consecutive_zeros >= size) {
                return start_pos;
            }
        }
        else {
            // 遇到非0值，重置计数器
            consecutive_zeros = 0;
            start_pos = -1;
        }
    }

    return -1;  // 未找到足够大的连续空间
}

#endif
