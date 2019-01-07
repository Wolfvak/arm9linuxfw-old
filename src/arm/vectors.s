#include <asm.h>
.align 2
.arm

ASM_FUNCTION vectors
    ldr pc, =XRQ_Reset         @ Reset
    ldr pc, =XRQ_Undefined     @ Undefined
    ldr pc, =XRQ_SoftwareInt   @ Software Interrupt
    ldr pc, =XRQ_PrefetchAbort @ Prefetch Abort
    ldr pc, =XRQ_DataAbort     @ Data Abort
    b .                        @ Reserved
    ldr pc, =irq_handler       @ IRQ
    ldr pc, =XRQ_FIQ           @ FIQ

    .pool

XRQ_Reset:
XRQ_Undefined:
XRQ_SoftwareInt:
XRQ_PrefetchAbort:
XRQ_DataAbort:
XRQ_FIQ:
    ldr r0, =0x18000000
    ldr r1, =0x18600000
    ldr r2, =0x43986739
    1:
        cmp r0, r1
        ldreq r0, =0x18000000
        strne r2, [r0], #4
        ror r2, #1
        b 1b
