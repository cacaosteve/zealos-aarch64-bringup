#pragma once
#include <stdarg.h>
#include <stddef.h>
typedef struct zc_file {
    const char *data;
    char *owned;
    size_t size, pos;
    int used;
} FILE;
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define EOF (-1)
#define stdout ((FILE *)1)
#define stderr ((FILE *)2)
#define snprintf zc_snprintf
#define vsnprintf zc_vsnprintf
#define fprintf zc_fprintf
#define printf zc_printf
#define puts zc_puts
#define fopen zc_fopen
#define fclose zc_fclose
#define fseek zc_fseek
#define ftell zc_ftell
#define fread zc_fread
int snprintf(char *, size_t, const char *, ...);
int vsnprintf(char *, size_t, const char *, va_list);
int fprintf(FILE *, const char *, ...);
int printf(const char *, ...);
int puts(const char *);
FILE *fopen(const char *, const char *);
int fclose(FILE *);
int fseek(FILE *, long, int);
long ftell(FILE *);
size_t fread(void *, size_t, size_t, FILE *);
