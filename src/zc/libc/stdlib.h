#pragma once
#include <stddef.h>
#define abort zc_abort
#define malloc zc_malloc
#define calloc zc_calloc
#define free zc_free
#define realloc zc_realloc
#define qsort zc_qsort
void abort(void) __attribute__((noreturn));
void *malloc(size_t);
void *calloc(size_t, size_t);
void free(void *);
void *realloc(void *, size_t);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));
