/* Small freestanding C support used only by the imported compiler. */
#include "aiwn_except.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void zc_output(const char *s);
extern void zc_fail(const char *s) __attribute__((noreturn));

void *memcpy(void *d, const void *s, size_t n) {
    unsigned char *a = d;
    const unsigned char *b = s;
    for (size_t i = 0; i < n; i++)
        a[i] = b[i];
    return d;
}
void *memmove(void *d, const void *s, size_t n) {
    unsigned char *a = d;
    const unsigned char *b = s;
    if ((uintptr_t)a < (uintptr_t)b)
        return memcpy(d, s, n);
    while (n) {
        --n;
        a[n] = b[n];
    }
    return d;
}
void *memset(void *d, int c, size_t n) {
    unsigned char *a = d;
    for (size_t i = 0; i < n; i++)
        a[i] = (unsigned char)c;
    return d;
}
int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = a, *y = b;
    for (size_t i = 0; i < n; i++)
        if (x[i] != y[i])
            return x[i] - y[i];
    return 0;
}
void *memchr(const void *s, int c, size_t n) {
    const unsigned char *p = s;
    for (size_t i = 0; i < n; i++)
        if (p[i] == (unsigned char)c)
            return (void *)(p + i);
    return NULL;
}
size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n])
        ++n;
    return n;
}
int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}
int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || !a[i])
            return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return 0;
}
char *strcpy(char *d, const char *s) {
    memcpy(d, s, strlen(s) + 1);
    return d;
}
char *strncpy(char *d, const char *s, size_t n) {
    size_t i = 0;
    for (; i < n && s[i]; i++)
        d[i] = s[i];
    for (; i < n; i++)
        d[i] = 0;
    return d;
}
char *strcat(char *d, const char *s) {
    strcpy(d + strlen(d), s);
    return d;
}
char *strchr(const char *s, int c) {
    do {
        if (*s == (char)c)
            return (char *)s;
    } while (*s++);
    return NULL;
}
char *strrchr(const char *s, int c) {
    const char *p = NULL;
    do {
        if (*s == (char)c)
            p = s;
    } while (*s++);
    return (char *)p;
}
char *strstr(const char *s, const char *n) {
    size_t len = strlen(n);
    for (; *s; s++)
        if (!strncmp(s, n, len))
            return (char *)s;
    return len ? NULL : (char *)s;
}

/* Diagnostic formatter. Length modifiers matter: the compiler passes both
 * ordinary ints and int64_t. Return the untruncated length like snprintf. */
static void emit(char *d, size_t cap, size_t *n, char c) {
    if (*n + 1 < cap)
        d[*n] = c;
    (*n)++;
}
int vsnprintf(char *d, size_t cap, const char *fmt, va_list ap) {
    size_t n = 0;
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            emit(d, cap, &n, *fmt);
            continue;
        }
        ++fmt;
        if (*fmt == '%') {
            emit(d, cap, &n, '%');
            continue;
        }
        char pad = ' ';
        unsigned width = 0;
        int ll = 0;
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + *fmt - '0';
            fmt++;
            if (width > 256)
                width = 256;
        }
        if (*fmt == 'l') {
            ll = 1;
            fmt++;
            if (*fmt == 'l') {
                ll = 2;
                fmt++;
            }
        } else if (*fmt == 'z') {
            ll = 1;
            fmt++;
        }
        if (*fmt == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s)
                emit(d, cap, &n, *s++);
        } else if (*fmt == 'c')
            emit(d, cap, &n, (char)va_arg(ap, int));
        else if (*fmt == 'd' || *fmt == 'i' || *fmt == 'u' || *fmt == 'x' || *fmt == 'X' ||
                 *fmt == 'p') {
            uint64_t v;
            int neg = 0, base = (*fmt == 'x' || *fmt == 'X' || *fmt == 'p') ? 16 : 10;
            if (*fmt == 'p')
                v = (uintptr_t)va_arg(ap, void *);
            else if (*fmt == 'd' || *fmt == 'i') {
                int64_t s = ll == 2 ? va_arg(ap, long long)
                            : ll    ? va_arg(ap, long)
                                    : va_arg(ap, int);
                neg = s < 0;
                v = neg ? 0 - (uint64_t)s : (uint64_t)s;
            } else
                v = ll == 2 ? va_arg(ap, unsigned long long)
                    : ll    ? va_arg(ap, unsigned long)
                            : va_arg(ap, unsigned);
            char buf[32];
            unsigned i = 0;
            const char *digits = *fmt == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
            do {
                buf[i++] = digits[v % (unsigned)base];
                v /= (unsigned)base;
            } while (v);
            if (neg)
                emit(d, cap, &n, '-');
            while (width > i + (unsigned)neg) {
                emit(d, cap, &n, pad);
                width--;
            }
            while (i)
                emit(d, cap, &n, buf[--i]);
        } else {
            emit(d, cap, &n, '%');
            if (*fmt)
                emit(d, cap, &n, *fmt);
            else
                break;
        }
    }
    if (cap)
        d[n < cap ? n : cap - 1] = 0;
    return (int)n;
}
int snprintf(char *d, size_t cap, const char *f, ...) {
    va_list a;
    va_start(a, f);
    int r = vsnprintf(d, cap, f, a);
    va_end(a);
    return r;
}
int fprintf(FILE *stream, const char *f, ...) {
    (void)stream;
    char b[512];
    va_list a;
    va_start(a, f);
    int r = vsnprintf(b, sizeof(b), f, a);
    va_end(a);
    zc_output(b);
    return r;
}
int printf(const char *f, ...) {
    char b[512];
    va_list a;
    va_start(a, f);
    int r = vsnprintf(b, sizeof(b), f, a);
    va_end(a);
    zc_output(b);
    return r;
}
int puts(const char *s) {
    zc_output(s);
    zc_output("\n");
    return 0;
}
void abort(void) { zc_fail("compiler abort"); }
void zc_assert_fail(const char *e, const char *f, int l) {
    fprintf(stderr, "assert %s:%d: %s\n", f, l, e);
    zc_fail("compiler assertion");
}

/* Used only to order compiler local-register candidates; insertion sort keeps
 * this support routine independent of the upstream QuickSort under test. */
void qsort(void *base, size_t n, size_t w, int (*cmp)(const void *, const void *)) {
    unsigned char *p = base;
    if (!w || n > SIZE_MAX / w)
        return;
    for (size_t i = 1; i < n; i++)
        for (size_t j = i; j && cmp(p + (j - 1) * w, p + j * w) > 0; j--)
            for (size_t b = 0; b < w; b++) {
                unsigned char t = p[j * w + b];
                p[j * w + b] = p[(j - 1) * w + b];
                p[(j - 1) * w + b] = t;
            }
}
double floor(double x) {
    double r;
    __asm__("frintm %d0, %d1" : "=w"(r) : "w"(x));
    return r;
}
/* Bootstrap math contract: exponent must be a finite integer. Unsupported
 * operations stop the compiler session instead of supplying false results. */
double pow(double a, double b) {
    if (!(b >= -65536 && b <= 65536) || floor(b) != b)
        zc_fail("fractional/large power unsupported");
    int64_t e = (int64_t)b;
    unsigned n = (unsigned)(e < 0 ? -e : e);
    double r = 1;
    if (e < 0)
        a = 1 / a;
    while (n) {
        if (n & 1)
            r *= a;
        a *= a;
        n >>= 1;
    }
    return r;
}
double fmod(double a, double b) {
    union {
        double f;
        uint64_t u;
    } x = {a}, y = {b};
    uint64_t sign = x.u & ((uint64_t)1 << 63);
    x.u &= ~((uint64_t)1 << 63);
    y.u &= ~((uint64_t)1 << 63);
    if ((x.u >> 52) == 0x7ff || (y.u >> 52) == 0x7ff || !y.u)
        zc_fail("nonfinite/zero floating remainder unsupported");
    while (x.f >= y.f) {
        double t = y.f;
        while (t <= x.f / 2)
            t *= 2;
        x.f -= t;
    }
    x.u |= sign;
    return x.f;
}
void __clear_cache(void *start, void *end) {
    uint64_t ctr;
    __asm__ volatile("mrs %0, ctr_el0" : "=r"(ctr));
    uintptr_t dline = 4ull << ((ctr >> 16) & 15), iline = 4ull << (ctr & 15);
    for (uintptr_t p = (uintptr_t)start & ~(dline - 1); p < (uintptr_t)end; p += dline)
        __asm__ volatile("dc cvau, %0" ::"r"(p) : "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    for (uintptr_t p = (uintptr_t)start & ~(iline - 1); p < (uintptr_t)end; p += iline)
        __asm__ volatile("ic ivau, %0" ::"r"(p) : "memory");
    __asm__ volatile("dsb ish\n\tisb" ::: "memory");
}
