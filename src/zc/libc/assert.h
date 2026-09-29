#pragma once
void zc_assert_fail(const char *, const char *, int) __attribute__((noreturn));
#define assert(x) ((x) ? (void)0 : zc_assert_fail(#x, __FILE__, __LINE__))
