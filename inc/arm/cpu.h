#pragma once

#include <common.h>
#include <asm.h>


// Status Register bits
#define SR_USR (0x10)
#define SR_FIQ (0x11)
#define SR_IRQ (0x12)
#define SR_SVC (0x13)
#define SR_ABT (0x17)
#define SR_UND (0x1B)
#define SR_SYS (0x1F)

#define SR_PMODE_MASK   (0xF)

#define SR_THUMB (1 << 5)
#define SR_NOFIQ (1 << 6)
#define SR_NOIRQ (1 << 7)

#define SR_NOINT (SR_NOFIQ | SR_NOIRQ)


#ifndef __ASSEMBLER__

typedef u32 irqsave_status;

static inline u32
get_cpsr(void) {
    u32 cpsr;
    asmv("mrs %0, cpsr\n\t"
        : "=r"(cpsr) :: "memory");
    return cpsr;
}

static inline void
set_cpsr_c(u32 cpsr) {
    asmv("msr cpsr_c, %0\n\t"
        :: "r"(cpsr) : "memory");
}

static inline void
enable_interrupts(void) {
    set_cpsr_c(get_cpsr() & ~SR_NOINT);
}

static inline void
disable_interrupts(void) {
    set_cpsr_c(get_cpsr() | SR_NOINT);
}

static inline irqsave_status
enter_critical_section(void) {
    irqsave_status stat = get_cpsr();
    set_cpsr_c(stat | SR_NOINT);
    return stat & SR_NOINT;
}

static inline void
leave_critical_section(irqsave_status stat) {
    set_cpsr_c(stat | (get_cpsr() & ~SR_NOINT));
}

static inline void
wait_for_interrupt(void) {
    asmv(
        "mcr p15, 0, %0, c7, c10, 4\n\t" // drain write buffer
        "mcr p15, 0, %0, c7, c0, 4\n\t" // wait for interrupt
        :: "r"(0) : "memory"
    );
}

#define ATOMIC_BLOCK for(irqsave_status _s = enter_critical_section(); _s != 0xFF; leave_critical_section(_s), _s = 0xFF)

#endif /* __ASSEMBLER__ */
