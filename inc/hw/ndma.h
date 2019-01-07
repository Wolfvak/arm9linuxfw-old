#ifndef NDMA_H
#define NDMA_H

#include <common.h>

#define NDMA_CHANNELS (8)
#define NDMA_BASE     (0x10002000)


#define REG_NDMA_GLOBAL_CONTROL   (*((vu32*)NDMA_BASE))

#define NDMA_GLOBAL_ENABLE        (BIT(0))
#define NDMA_GLOBAL_ROUNDROBIN(n) (BIT(31) | ((n) & 0xF) << 16)


#define REG_NDMA_BASE           ((vu32*)NDMA_BASE)
#define REG_NDMA_SRC(n)         (REG_NDMA_BASE[((n)*7) + 1])
#define REG_NDMA_DST(n)         (REG_NDMA_BASE[((n)*7) + 2])
#define REG_NDMA_XFER_COUNT(n)  (REG_NDMA_BASE[((n)*7) + 3])
#define REG_NDMA_BLK_COUNT(n)   (REG_NDMA_BASE[((n)*7) + 4])
#define REG_NDMA_BLK_TIMING(n)  (REG_NDMA_BASE[((n)*7) + 5])
#define REG_NDMA_FILL_DATA(n)   (REG_NDMA_BASE[((n)*7) + 6])
#define REG_NDMA_CONTROL(n)     (REG_NDMA_BASE[((n)*7) + 7])

#define NDMA_BLK_TIMING_0   (0)

#define NDMA_DST_UPDATE_INC (0 << 10)
#define NDMA_DST_UPDATE_DEC (1 << 10)
#define NDMA_DST_UPDATE_FIX (2 << 10)
#define NDMA_DST_RELOAD     (BIT(12))

#define NDMA_SRC_UPDATE_INC (0 << 13)
#define NDMA_SRC_UPDATE_DEC (1 << 13)
#define NDMA_SRC_UPDATE_FIX (2 << 13)
#define NDMA_SRC_UPDATE_NA  (3 << 13)
#define NDMA_SRC_RELOAD     (BIT(15))

#define NDMA_BURST_COUNT(n) ((n) << 16)

#define NDMA_STARTUP_TIMER0   (0 << 24)
#define NDMA_STARTUP_TIMER1   (1 << 24)
#define NDMA_STARTUP_TIMER2   (2 << 24)
#define NDMA_STARTUP_TIMER3   (3 << 24)
#define NDMA_STARTUP_CTRCARD0 (4 << 24)
#define NDMA_STARTUP_CTRCARD1 (5 << 24)
#define NDMA_STARTUP_EMMC     (7 << 24)
#define NDMA_STARTUP_AES_IN   (8 << 24)
#define NDMA_STARTUP_AES_OUT  (9 << 24)
#define NDMA_STARTUP_SHA      (10 << 24)
#define NDMA_STARTUP_AES_SHA  (15 << 24)

#define NDMA_IMMEDIATE      (BIT(28))
#define NDMA_REPEATING      (BIT(29))

#define NDMA_IRQ            (BIT(30))
#define NDMA_ENABLE         (BIT(31))


void NDMA_Reset(void);

void NDMA_CopyAsync(u32 *dst, const u32 *src, size_t len);
void NDMA_Copy(u32 *dst, const u32 *src, size_t len);

void NDMA_FillAsync(u32 *dst, u32 fill, size_t len);
void NDMA_Fill(u32 *dst, u32 fill, size_t len);

void NDMA_Operation(u32 *dst, u32 sv, u32 flags, size_t len);

#endif // NDMA_H
