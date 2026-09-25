/*
 * AArch64 exception vectors (VBAR_EL1).
 * Each entry is 0x80 bytes; table must be 2KiB-aligned.
 */
.section .text.vectors, "ax"
.align 11
.global exception_vectors
exception_vectors:

/* Current EL with SP0 */
.align 7
    b exc_sync_el1
.align 7
    b exc_irq_el1
.align 7
    b exc_fiq_invalid
.align 7
    b exc_serror_invalid

/* Current EL with SPx */
.align 7
    b exc_sync_el1
.align 7
    b exc_irq_el1
.align 7
    b exc_fiq_invalid
.align 7
    b exc_serror_invalid

/* Lower EL using AArch64 */
.align 7
    b exc_sync_invalid
.align 7
    b exc_irq_invalid
.align 7
    b exc_fiq_invalid
.align 7
    b exc_serror_invalid

/* Lower EL using AArch32 */
.align 7
    b exc_sync_invalid
.align 7
    b exc_irq_invalid
.align 7
    b exc_fiq_invalid
.align 7
    b exc_serror_invalid

.macro EXC_ENTRY name, handler
\name:
    sub sp, sp, #256
    stp x0, x1, [sp, #0]
    stp x2, x3, [sp, #16]
    stp x4, x5, [sp, #32]
    stp x6, x7, [sp, #48]
    stp x8, x9, [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]
    mov x0, sp
    bl \handler
    ldr x30, [sp, #240]
    ldp x28, x29, [sp, #224]
    ldp x26, x27, [sp, #208]
    ldp x24, x25, [sp, #192]
    ldp x22, x23, [sp, #176]
    ldp x20, x21, [sp, #160]
    ldp x18, x19, [sp, #144]
    ldp x16, x17, [sp, #128]
    ldp x14, x15, [sp, #112]
    ldp x12, x13, [sp, #96]
    ldp x10, x11, [sp, #80]
    ldp x8, x9, [sp, #64]
    ldp x6, x7, [sp, #48]
    ldp x4, x5, [sp, #32]
    ldp x2, x3, [sp, #16]
    ldp x0, x1, [sp, #0]
    add sp, sp, #256
    eret
.endm

EXC_ENTRY exc_sync_el1, exc_handle_sync
EXC_ENTRY exc_irq_el1, exc_handle_irq

exc_sync_invalid:
exc_irq_invalid:
exc_fiq_invalid:
exc_serror_invalid:
    mov x0, #0
    bl exc_handle_fatal
1:  wfi
    b 1b
