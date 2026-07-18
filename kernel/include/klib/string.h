#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strlen(const char *s);
int strcmp(char *s1, char *s2);
char *strcat(char *dest, char *src);
void itoa(int num, char *str);
int atoi(const char *str);

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

#endif
