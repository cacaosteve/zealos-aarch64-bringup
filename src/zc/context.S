.text
.global zc_setjmp
.type zc_setjmp, %function
zc_setjmp:
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x1, sp
    str x1, [x0, #96]
    stp d8, d9, [x0, #112]
    stp d10, d11, [x0, #128]
    stp d12, d13, [x0, #144]
    stp d14, d15, [x0, #160]
    mrs x1, fpcr
    str x1, [x0, #176]
    mrs x1, fpsr
    str x1, [x0, #184]
    mov x0, #0
    ret
.global zc_longjmp
.type zc_longjmp, %function
zc_longjmp:
    ldp x19, x20, [x0, #0]
    ldp x21, x22, [x0, #16]
    ldp x23, x24, [x0, #32]
    ldp x25, x26, [x0, #48]
    ldp x27, x28, [x0, #64]
    ldp x29, x30, [x0, #80]
    ldr x2, [x0, #96]
    mov sp, x2
    ldp d8, d9, [x0, #112]
    ldp d10, d11, [x0, #128]
    ldp d12, d13, [x0, #144]
    ldp d14, d15, [x0, #160]
    ldr x2, [x0, #176]
    msr fpcr, x2
    ldr x2, [x0, #184]
    msr fpsr, x2
    cmp x1, #0
    csinc x0, x1, xzr, ne
    ret
.section .note.GNU-stack,"",%progbits
