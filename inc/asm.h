#pragma once

#ifdef __ASSEMBLER__
/* Function declaration macro */
.macro ASM_FUNCTION f
    .section .text.\f, "ax", %progbits

    .global \f
    .type \f, %function
    .align 2
\f:
.endm


.macro NOP_SLED c
    .rept \c
    mov r0, r0
    .endr
.endm

#endif // __ASSEMBLER__
