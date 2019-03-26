#include "common.h"
#include "interrupt.h"

#include "arm/cpu.h"
#include "hw/irq.h"

#define REG_IRQ ((vu32*)0x10001000)

static interrupt_callback interrupts[IRQ_COUNT];

static inline int
irq_pending(void)
{
    u32 pend = REG_IRQ[1];
    return pend ? (31 - __builtin_clz(pend)) : -1;
}

static inline void
irq_ack(u32 irqn)
{
    REG_IRQ[1] = BIT(irqn);
}

void __attribute__((interrupt("irq")))
irq_handler(void)
{
    int pend;
    while((pend = irq_pending()) >= 0) {
        if (interrupts[pend] != NULL) {
            int res = (interrupts[pend])(pend);
            if (res == IRQ_HANDLED)
                irq_ack(pend);
        } else {
            irq_ack(pend);
        }
    }
}

void irq_reset(void)
{
    // clear the interrupt handler table
    for (size_t i = 0; i < ARRAY_SIZE(interrupts); i++)
        interrupts[i] = NULL;

    // disable all interrupts, acknowledge all pending interrupts
    REG_IRQ[0] = 0;
    do {
        REG_IRQ[1] = 0xFFFFFFFFUL;
    } while(REG_IRQ[1]);
}

void irq_register(u32 irqn, interrupt_callback cb)
{
    assert(irqn < IRQ_COUNT);

    interrupts[irqn] = cb;
    REG_IRQ[1]  = BIT(irqn);
    REG_IRQ[0] |= BIT(irqn);
}

void irq_remove(u32 irqn)
{
    assert(irqn < IRQ_COUNT);

    interrupts[irqn] = NULL;
    REG_IRQ[1]  = BIT(irqn);
    REG_IRQ[0] &= ~BIT(irqn);
}
