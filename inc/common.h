#pragma once

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define asmv __asm__ volatile

#define assert(x) ((x) ? (void)0 : __builtin_trap())

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef volatile u8  vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef volatile s8  vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

#endif // __ASSEMBLER__

#define BIT(x)      (1 << (x))
