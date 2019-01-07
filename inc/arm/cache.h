#pragma once

#include <common.h>

static inline void
cache_drainwritebuf(void) {
    asmv("mcr p15, 0, %0, c7, c10, 4\n\t"
        :: "r"(0) : "memory");
}

static inline void
cache_invalidate_ic(void) {
    asmv("mcr p15, 0, %0, c7, c5, 0\n\t"
        :: "r"(0) : "memory");

    cache_drainwritebuf();
}

static inline void
cache_invalidate_ic_range(const void *base, u32 len) {
    u32 addr = (u32)base & ~0x1F;
    len >>= 5;

    do {
        asmv("mcr p15, 0, %0, c7, c5, 1\n\t"
            :: "r"(addr) : "memory");

        addr += 0x20;
    } while(len--);

    cache_drainwritebuf();
}

static inline void
cache_invalidate_dc(void) {
    asmv("mcr p15, 0, %0, c7, c6, 0\n\t"
        :: "r"(0) : "memory");
    cache_drainwritebuf();
}

static inline void
cache_invalidate_dc_range(const void *base, u32 len) {
    u32 addr = (u32)base & ~0x1F;
    len >>= 5;

    do {
        asmv("mcr p15, 0, %0, c7, c6, 1"
            :: "r"(addr) : "memory");
        addr += 0x20;
    } while(len--);

    cache_drainwritebuf();
}

static inline void
cache_writeback_dc(void) {
    u32 seg = 0, ind;
    do {
        ind = 0;
        do {
            asmv("mcr p15, 0, %0, c7, c10, 2\n\t"
               :: "r"(seg | ind) : "memory");

            ind += 0x20;
        } while(ind < 0x400);
        seg += 0x40000000;
    } while(seg != 0);

    cache_drainwritebuf();
}

static inline
void cache_writeback_dc_range(const void *base, u32 len) {
    u32 addr = (u32)base & ~0x1F;
    len >>= 5;

    do {
        asmv("mcr p15, 0, %0, c7, c10, 1"
            :: "r"(addr) : "memory");

        addr += 0x20;
    } while(len--);

    cache_drainwritebuf();
}

static inline
void cache_writeback_invalidate_dc(void) {
    u32 seg = 0, ind;
    do {
        ind = 0;
        do {
            asmv("mcr p15, 0, %0, c7, c14, 2\n\t"
                :: "r"(seg | ind) : "memory");

            ind += 0x20;
        } while(ind < 0x400);
        seg += 0x40000000;
    } while(seg != 0);

    cache_drainwritebuf();
}

static inline
void cache_writeback_invalidate_dc_range(const void *base, u32 len) {
    u32 addr = (u32)base & ~0x1F;
    len >>= 5;

    do {
        asmv("mcr p15, 0, %0, c7, c14, 1"
            :: "r"(addr) : "memory");

        addr += 0x20;
    } while(len--);

    cache_drainwritebuf();
}
