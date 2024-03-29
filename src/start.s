#include "asm.h"
#include "arm/cpu.h"

#define IRQ_STACK_SIZE (256)
#define SYS_STACK_SIZE (8192)

.section .bootstrap, "ax"
.global __start
.align 2
__start:
    @ Switch to supervisor mode and disable interrupts
    msr cpsr_c, #(SR_SVC | SR_NOINT)


    @ Make sure binary is written back to main memory and out of caches
    bic r0, pc, #0x1F
    add r1, r0, #0x10000
    1:
        ldr r2, [r0], #0x20
        cmp r0, r1
        blo 1b

    @ Cache maintenance operations
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4  @ drain write buffer (data memory barrier)
    mcr p15, 0, r0, c7, c6, 0   @ flush data cache
    mcr p15, 0, r0, c7, c5, 0   @ flush instruction cache


    @ Disable the MPU, caches, TCMs, set high exception vectors
    ldr r0, =0x2078
    mcr p15, 0, r0, c1, c0, 0
    NOP_SLED 2


    @ Setup Tightly Coupled Memory
    ldr r0, =0x4000000A @ DTCM @ 0x40000000 / 16KB (16KB mirror)
    ldr r1, =0x00000024 @ ITCM @ 0x00000000 / 32KB (128MB mirror)
    mcr p15, 0, r0, c9, c1, 0
    mcr p15, 0, r1, c9, c1, 1
    NOP_SLED 2


    @ Enable TCMs
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #0x50000
    mcr p15, 0, r0, c1, c0, 0
    NOP_SLED 2


    @ Relocate vectors
    ldr r0, =__vector_lma
    ldr r1, =__vector_s
    ldr r2, =__vector_e
    bl BootstrapReloc

    @ Relocate executable code
    ldr r0, =__text_lma
    ldr r1, =__text_s
    ldr r2, =__text_e
    bl BootstrapReloc

    @ Relocate data and rodata
    ldr r0, =__data_lma
    ldr r1, =__data_s
    ldr r2, =__data_e
    bl BootstrapReloc


    @ Clear BSS
    mov r0, #0
    ldr r1, =__bss_s
    ldr r2, =__bss_e
    1:
        cmp r1, r2
        strlo r0, [r1], #4
        blo 1b


    @ Branch to main startup code
    ldr lr, =start_itcm
    bx lr


@ void BootstrapReloc(void *lma, void *vma_start, void *vma_end)
@ equivalent to memcpy(vma_start, lma, vma_end - vma_start)
@ assumes all pointers are 16byte aligned
BootstrapReloc:
    cmp r1, r2
    ldmloia r0!, {r3-r6}
    stmloia r1!, {r3-r6}
    blo BootstrapReloc
    bx lr

.pool


ASM_FUNCTION start_itcm
    @ Setup stacks
    msr cpsr_c, #(SR_IRQ | SR_NOINT)
    ldr sp, =(irq_stack_bottom + IRQ_STACK_SIZE)

    msr cpsr_c, #(SR_SYS | SR_NOINT)
    ldr sp, =(sys_stack_bottom + SYS_STACK_SIZE)


    @ MPU Regions:
    @ N | Name    | Start      | End        | Data  | Inst  | IC | DC | DB
    @ 0 | ITCM    | 0x01FF8000 | 0x07FFFFFF | RW_NA | RO_NA | n  | n  | n
    @ 1 | AHBRAM  | 0x08000000 | 0x08FFFFFF | RW_NA | RO_NA | y  | y  | y
    @ 2 | MMIO    | 0x10000000 | 0x101FFFFF | RW_NA | NA_NA | n  | n  | n
    @ 3 | VRAM    | 0x18000000 | 0x187FFFFF | RW_NA | NA_NA | n  | n  | n
    @ 4 | AXIRAM  | 0x1FF00000 | 0x1FFFFFFF | RW_NA | NA_NA | n  | n  | n
    @ 5 | FCRAM   | 0x20000000 | 0x2FFFFFFF | RW_NA | NA_NA | n  | n  | n
    @ 6 | DTCM    | 0x40000000 | 0x40003FFF | RW_NA | NA_NA | n  | n  | n
    @ 7 | BootROM | 0xFFFF0000 | 0xFFFF7FFF | RO_NA | RO_NA | y  | n  | n

    mov r0, #0b10000010 @ Instruction Cachable
    mov r1, #0b00000010 @ Data Cachable
    mov r2, #0b00000010 @ Data Bufferable

    ldr r3, =0x51111111 @ Data Access Permissions
    ldr r4, =0x50000055 @ Instruction Access Permissions

    mcr p15, 0, r0, c2, c0, 1
    mcr p15, 0, r1, c2, c0, 0
    mcr p15, 0, r2, c3, c0, 0

    mcr p15, 0, r3, c5, c0, 2
    mcr p15, 0, r4, c5, c0, 3

    ldr r8, =mpu_regions
    ldmia r8, {r0-r7}
    mcr p15, 0, r0, c6, c0, 0
    mcr p15, 0, r1, c6, c1, 0
    mcr p15, 0, r2, c6, c2, 0
    mcr p15, 0, r3, c6, c3, 0
    mcr p15, 0, r4, c6, c4, 0
    mcr p15, 0, r5, c6, c5, 0
    mcr p15, 0, r6, c6, c6, 0
    mcr p15, 0, r7, c6, c7, 0


    @ Enable MPU and caches, use high vectors
    ldr r1, =0x3005
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, r1
    mcr p15, 0, r0, c1, c0, 0
    NOP_SLED 2


    @ Fix SDMC
    mov r0, #0x10000000
    ldrh r1, [r0, #0x20]
    bic r1, r1, #0x001
    orr r1, r1, #0x200
    strh r1, [r0, #0x20]


    @ Branch to C code
    ldr lr, =pxicmd_mainloop
    bx lr


ASM_FUNCTION sdmmc_read_stuff
@ void sdmmc_read_stuff(vu32 *reg, u32 *dest)
@ reads 128 words in one go
    push {r4-r8, lr}

    .rept 16
    ldr r2, [r0]
    ldr r3, [r0]
    ldr r4, [r0]
    ldr r5, [r0]
    ldr r6, [r0]
    ldr r7, [r0]
    ldr r8, [r0]
    ldr lr, [r0]
    stmia r1!, {r2-r8, lr}
    .endr

    pop {r4-r8, pc}


.section .bss.stacks
.align 3

.global irq_stack_bottom
irq_stack_bottom:
    .space (IRQ_STACK_SIZE)

.global sys_stack_bottom
sys_stack_bottom:
    .space (SYS_STACK_SIZE)


.section .rodata.mpu_regions
.align 2

.global mpu_regions
mpu_regions:
    .word 0x01FF8035 @ ITCM
    .word 0x08000027 @ AHBRAM
    .word 0x10000029 @ MMIO
    .word 0x1800002D @ VRAM
    .word 0x1FF00027 @ AXIRAM
    .word 0x20000037 @ FCRAM
    .word 0x4000001B @ DTCM
    .word 0xFFFF001D @ BootROM
