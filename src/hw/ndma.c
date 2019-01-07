#include <common.h>
#include <interrupt.h>

#include <arm/cpu.h>
#include <hw/irq.h>
#include <hw/ndma.h>

#define NDMA_COPY_CHANNEL (0)
#define NDMA_FILL_CHANNEL (1)
#define NDMA_OP_CHANNEL   (2)

void NDMA_Reset(void)
{
    for (int i = 0; i < 8; i++) {
        REG_NDMA_CONTROL(i) = 0;
        irq_register(IRQ_DMA0 + i, NULL);
    }
    REG_NDMA_GLOBAL_CONTROL = NDMA_GLOBAL_ENABLE;
}

void NDMA_CopyAsync(u32 *dst, const u32 *src, size_t len)
{
    REG_NDMA_SRC(NDMA_COPY_CHANNEL) = (u32)src;
    REG_NDMA_DST(NDMA_COPY_CHANNEL) = (u32)dst;

    REG_NDMA_BLK_COUNT(NDMA_COPY_CHANNEL) = len / sizeof(u32);
    REG_NDMA_BLK_TIMING(NDMA_COPY_CHANNEL) = NDMA_BLK_TIMING_0 | 1;

    REG_NDMA_CONTROL(NDMA_COPY_CHANNEL) = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_INC | NDMA_IMMEDIATE | NDMA_IRQ | NDMA_ENABLE;
}

void NDMA_Copy(u32 *dst, const u32 *src, size_t len)
{
    NDMA_CopyAsync(dst, src, len);
    while(REG_NDMA_CONTROL(NDMA_COPY_CHANNEL) & NDMA_ENABLE)
        wait_for_interrupt();
}

void NDMA_FillAsync(u32 *dst, u32 fill, size_t len)
{
    REG_NDMA_DST(NDMA_FILL_CHANNEL) = (u32)dst;

    REG_NDMA_FILL_DATA(NDMA_FILL_CHANNEL)  = fill;
    REG_NDMA_BLK_COUNT(NDMA_FILL_CHANNEL)  = len / sizeof(u32);
    REG_NDMA_BLK_TIMING(NDMA_FILL_CHANNEL) = NDMA_BLK_TIMING_0 | 1;

    REG_NDMA_CONTROL(NDMA_FILL_CHANNEL) = NDMA_DST_UPDATE_INC | NDMA_SRC_UPDATE_NA | NDMA_IMMEDIATE | NDMA_IRQ | NDMA_ENABLE;
}

void NDMA_Fill(u32 *dst, u32 fill, size_t len)
{
    NDMA_FillAsync(dst, fill, len);
    while(REG_NDMA_CONTROL(NDMA_FILL_CHANNEL) & NDMA_ENABLE)
        wait_for_interrupt();
}

void NDMA_OperationAsync(u32 *dst, u32 sv, u32 flags, size_t len)
{
    REG_NDMA_DST(NDMA_OP_CHANNEL) = (u32)dst;
    REG_NDMA_SRC(NDMA_OP_CHANNEL) = sv;

    REG_NDMA_FILL_DATA(NDMA_OP_CHANNEL) = sv;
    REG_NDMA_BLK_COUNT(NDMA_OP_CHANNEL) = len / sizeof(u32);
    REG_NDMA_BLK_TIMING(NDMA_OP_CHANNEL) = NDMA_BLK_TIMING_0 | 1;

    REG_NDMA_CONTROL(NDMA_OP_CHANNEL) = NDMA_IMMEDIATE | NDMA_IRQ | NDMA_ENABLE | flags;
}

void NDMA_Operation(u32 *dst, u32 sv, u32 flags, size_t len)
{
    NDMA_OperationAsync(dst, sv, flags, len);
    while(REG_NDMA_CONTROL(NDMA_OP_CHANNEL) & NDMA_ENABLE)
        wait_for_interrupt();
}
