#pragma once

#include <common.h>

#define PXI_BASE (0x10008000)

#define REG_PXI_SYNC_RECV  (*((vu8*)(PXI_BASE + 0x00)))
#define REG_PXI_SYNC_SEND  (*((vu8*)(PXI_BASE + 0x01)))
#define REG_PXI_SYNC_CNT   (*((vu8*)(PXI_BASE + 0x03)))

#define REG_PXI_CNT        (*((vu16*)(PXI_BASE + 0x04)))

#define REG_PXI_SEND       (*((vu32*)(PXI_BASE + 0x08)))
#define REG_PXI_RECV       (*((vu32*)(PXI_BASE + 0x0C)))


#define PXI_SEND_FIFO_EMPTY      (BIT(0))
#define PXI_SEND_FIFO_FULL       (BIT(1))
#define PXI_SEND_FIFO_EMPTY_IRQ  (BIT(2))
#define PXI_SEND_FIFO_CLEAR      (BIT(3))
#define PXI_RECV_FIFO_EMPTY      (BIT(8))
#define PXI_RECV_FIFO_FULL       (BIT(9))
#define PXI_RECV_FIFO_NEMPTY_IRQ (BIT(10))
#define PXI_FIFO_ERROR_ACK       (BIT(14))
#define PXI_FIFO_ENABLE          (BIT(15))

#define PXI_SYNC_IRQ_ENABLE      (BIT(7))

#define PXI_FIFO_WIDTH           (16)

#ifndef __ASSEMBLER__

void pxi_reset(void);

static inline bool pxi_sendfifofull(void) {
    return (REG_PXI_CNT & PXI_SEND_FIFO_FULL);
}

static inline bool pxi_sendfifoempty(void) {
    return (REG_PXI_CNT & PXI_SEND_FIFO_EMPTY);
}

static inline bool pxi_recvfifofull(void) {
    return (REG_PXI_CNT & PXI_RECV_FIFO_FULL);
}

static inline bool pxi_recvfifoempty(void) {
    return (REG_PXI_CNT & PXI_RECV_FIFO_EMPTY);
}

static inline u8 pxi_recvsync(void) {
    return REG_PXI_SYNC_RECV;
}

static inline void pxi_sendsync(u8 b) {
    REG_PXI_SYNC_SEND = b;
}

static inline u32 pxi_recv(void) {
    while(pxi_recvfifoempty());
    return REG_PXI_RECV;
}

static inline void pxi_send(u32 w) {
    while(pxi_sendfifofull());
    REG_PXI_SEND = w;
}

static inline void pxi_trigger_sync11(void) {
    REG_PXI_SYNC_CNT |= 1 << 5;
}

#endif // __ASSEMBLER__
