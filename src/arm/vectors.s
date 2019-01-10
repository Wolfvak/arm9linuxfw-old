#include <asm.h>
.align 2
.arm

.macro TRAP_ENTRY xrq_id
    msr cpsr_f, #(\xrq_id << 29)    @ preserve exception source (idea grabbed from fb3ds)
.endm

.section .vector, "ax"
.global vector
vector:
    ldr pc, _irq_vector
    _irq_vector: .word irq_handler
    ldr pc, _fiq_vector
    _fiq_vector: .word fiq_handler
    ldr pc, _svc_vector
    _svc_vector: .word svc_handler
    ldr pc, _undef_vector
    _undef_vector: .word und_handler
    ldr pc, _prefetch_vector
    _prefetch_vector: .word pre_handler
    ldr pc, _data_vector
    _data_vector: .word dat_handler


.section .text.vector_entry, "ax"
svc_handler:
    TRAP_ENTRY 0
    b _exception_fatal

und_handler:
    TRAP_ENTRY 1
    b _exception_fatal

pre_handler:
    TRAP_ENTRY 2
    b _exception_fatal

dat_handler:
    TRAP_ENTRY 3
    b _exception_fatal

fiq_handler:
    TRAP_ENTRY 4

_exception_fatal:
    ldr sp, =(_fatal_stk - 18*4)
    stmia sp, {r0-r7}               @ store the non-banked GPRs

    mrs r1, cpsr
    orr r0, r1, #0xC0
    msr cpsr_c, r0                  @ disable interrupts

    lsr r0, r1, #29                 @ r0 = fatal exception source

    mrs r2, spsr
    str r2, [sp, #16*4]             @ store exception PSR

    ands r2, r2, #0xF
    orreq r2, r2, #0xF              @ if the abort happened in USR
                                    @ switch to SYS instead

    orr r2, r2, #0xD0               @ with interrupts disabled
    add r3, sp, #8*4
    msr cpsr_c, r2
    nop
    stmia r3, {r8-r14}              @ store remaining prev mode registers
                                    @ including banked FIQ if needed
    msr cpsr_c, r1                  @ and return back to the exception mode
    nop

    str lr, [sp, #15*4]             @ store exception PC
    mov r1, sp
    bl handle_fatal_error

    mov r0, #0
    1:
        mcr p15, 0, r0, c7, c0, 4
        b 1b


.align 3
_fatal_stack_bottom:
    .space (512 * 4)
_fatal_stk:
