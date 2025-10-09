#ifndef _TYPE_H
#define _TYPE_H

#define NULL ((void*)0)
#define true 1
#define false 0

#define _packed __attribute__((packed))

typedef _Bool bool;
typedef unsigned int size_t;
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef void uint0_t;
typedef void (*callbk)(uint32_t);
typedef uint64_t time_t;

#endif