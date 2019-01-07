#pragma once

#include <common.h>

typedef int (*interrupt_callback)(u32 irqn);

void irq_reset(void);

void irq_register(u32 irqn, interrupt_callback cb);
void irq_remove(u32 irqn);
