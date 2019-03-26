#include "common.h"
#include "hw/pxi.h"

void
pxi_reset(void)
{
    // disable SYNC interrupt
    REG_PXI_SYNC_CNT = 0;

    // enable and clear send FIFO
    REG_PXI_CNT = PXI_SEND_FIFO_CLEAR | PXI_FIFO_ENABLE;

    // clear recv FIFO
    for (int i = 0; i < PXI_FIFO_WIDTH; i++)
        REG_PXI_RECV;

    // reset PXI
    REG_PXI_CNT = 0;

    // enable SYNC and RECV AVAILABLE interrupts by default
    REG_PXI_CNT = PXI_RECV_FIFO_AVAIL_IRQ | PXI_FIFO_ENABLE;
    REG_PXI_SYNC_CNT = PXI_SYNC_IRQ_ENABLE;

    REG_PXI_SYNC_SEND = 0;
    return;
}
