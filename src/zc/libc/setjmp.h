#pragma once
#include <stdint.h>
typedef uint64_t jmp_buf[24]; /* x19-x30, SP, padding, d8-d15, FPCR, FPSR */
int zc_setjmp(jmp_buf) __attribute__((returns_twice));
void zc_longjmp(jmp_buf, int) __attribute__((noreturn));
#define setjmp zc_setjmp
#define longjmp zc_longjmp
