#pragma once

#define IRQ_DMA0				(0)
#define IRQ_DMA1				(1)
#define IRQ_DMA2				(2)
#define IRQ_DMA3				(3)
#define IRQ_DMA4				(4)
#define IRQ_DMA5				(5)
#define IRQ_DMA6				(6)
#define IRQ_DMA7				(7)
#define IRQ_TIMER0				(8)
#define IRQ_TIMER1				(9)
#define IRQ_TIMER2				(10)
#define IRQ_TIMER3				(11)
#define IRQ_PXI_SYNC			(12)
#define IRQ_PXI_RECV_NOT_EMPTY	(14)
#define IRQ_SDIO1				(16)
#define IRQ_SDIO1_ASYNC			(17)
#define IRQ_SDIO3				(18)
#define IRQ_SDIO3_ASYNC			(19)

#define IRQ_COUNT				(29)

#define IRQ_DMA(n)				((n) + IRQ_DMA0)
#define IRQ_TIMER(n)			((n) + IRQ_TIMER0)
