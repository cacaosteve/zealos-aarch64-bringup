#pragma once
#include <stddef.h>
#define memcpy zc_memcpy
#define memmove zc_memmove
#define memset zc_memset
#define memcmp zc_memcmp
#define memchr zc_memchr
#define strlen zc_strlen
#define strcmp zc_strcmp
#define strncmp zc_strncmp
#define strcpy zc_strcpy
#define strncpy zc_strncpy
#define strcat zc_strcat
#define strchr zc_strchr
#define strrchr zc_strrchr
#define strstr zc_strstr
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
void *memchr(const void *, int, size_t);
size_t strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);
char *strcat(char *, const char *);
char *strchr(const char *, int);
char *strrchr(const char *, int);
char *strstr(const char *, const char *);
