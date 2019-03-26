#pragma once

#include "common.h"

#define IRQ_HANDLED	(0)
#define IRQ_AGAIN	(1)

typedef int (*interrupt_callback)(u32 irqn);

void irq_reset(void);

void irq_register(u32 irqn, interrupt_callback cb);
void irq_remove(u32 irqn);
