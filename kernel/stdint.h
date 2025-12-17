#ifndef _TYPE_H
#define _TYPE_H

#define NULL ((void*)0)

/* Booleans */
/* _Bool is a C99 built-in type. better to use it than int */
typedef _Bool bool;
#define true 1
#define false 0

/* Attributes */
#define _packed __attribute__((packed))

/* * Fixed Width Integers
 * NOTE: explicitly use 'signed char' for int8_t. 
 * Just 'char' can be unsigned on some architectures (like ARM).
 */
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

/* * System Types
 * size_t depends on your architecture (32 vs 64 bit).
 * If you are on 32-bit x86, 'unsigned int' is correct.
 */
typedef unsigned int       size_t;
typedef uint64_t           time_t;

/* * Custom OS Types
 * 'uint0_t' is non-standard but valid if you want an alias for void.
 */
//typedef void               uint0_t;
//typedef void (*callbk)(uint32_t); 

#endif