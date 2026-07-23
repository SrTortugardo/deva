#ifndef STRING_H
#define STRING_H

/* manipulacion de cadenas y memoria */
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dst, const char *src);
char *strchr(const char *s, int c);
void *memset(void *s, int c, int n);
void *memcpy(void *dst, const void *src, int n);

#endif
