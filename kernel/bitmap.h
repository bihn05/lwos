#ifndef _BITMAP_H
#define _BITMAP_H

#include <type.h>
#include <string.h>
#include <assert.h>

typedef struct bitmap_t{
	uint8_t *bits;
	uint32_t length;
	uint32_t offset;
} bitmap_t;

// 初始化位图
void bitmap_init(bitmap_t* map, char* bits, uint32_t length, uint32_t start)
{
	memset(bits, 0, length);
	bitmap_make(map, bits, length, start);
}
// 构造位图
void bitmap_make(bitmap_t* map, char* bits, uint32_t length, uint32_t offset)
{
	map->bits = bits;
	map->length = length;
	map->offset = offset;
}
// 测试位图的某一位是否为 1
bool bitmap_test(bitmap_t* map, idx_t index)
{
    assert(index >= map->offset);

    // 得到位图的索引
    idx_t idx = index - map->offset;

    // 位图数组中的字节
    uint32_t bytes = idx / 8;

    // 该字节中的那一位
    uint8_t bits = idx % 8;

    assert(bytes < map->length);

    // 返回那一位是否等于 1
    return (map->bits[bytes] & (1 << bits));
}
// 设置位图某位的值
void bitmap_set(bitmap_t* map, idx_t index, bool value)
{
    // value 必须是二值的
    assert(value == 0 || value == 1);

    assert(index >= map->offset);

    // 得到位图的索引
    idx_t idx = index - map->offset;

    // 位图数组中的字节
    uint32_t bytes = idx / 8;

    // 该字节中的那一位
    uint8_t bits = idx % 8;
    if (value)
    {
        // 置为 1
        map->bits[bytes] |= (1 << bits);
    }
    else
    {
        // 置为 0
        map->bits[bytes] &= ~(1 << bits);
    }
}
// 从位图中得到连续的 count 位
int bitmap_scan(bitmap_t* map, uint32_t count)
{
    int start = EOF;                 // 标记目标开始的位置
    uint32_t bits_left = map->length * 8; // 剩余的位数
    uint32_t next_bit = 0;                // 下一个位
    uint32_t counter = 0;                 // 计数器

    // 从头开始找
    while (bits_left-- > 0)
    {
        if (!bitmap_test(map, map->offset + next_bit))
        {
            // 如果下一个位没有占用，则计数器加一
            counter++;
        }
        else
        {
            // 否则计数器置为 0，继续寻找
            counter = 0;
        }

        // 下一位，位置加一
        next_bit++;

        // 找到数量一致，则设置开始的位置，结束
        if (counter == count)
        {
            start = next_bit - count;
            break;
        }
    }

    // 如果没找到，则返回 EOF(END OF FILE)
    if (start == EOF)
        return EOF;

    // 否则将找到的位，全部置为 1
    bits_left = count;
    next_bit = start;
    while (bits_left--)
    {
        bitmap_set(map, map->offset + next_bit, true);
        next_bit++;
    }

    // 然后返回索引
    return start + map->offset;
}
#endif