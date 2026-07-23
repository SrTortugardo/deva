#include <string.h>
#include <syscall.h>

int strlen(const char *s) {
  int n = 0;
  while (s[n]) { /* recorre hasta el nulo */
    n++;
  }
  return n;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) { /* avanzan iguales */
    s1++;
    s2++;
  }
  /* diferencia del primer carácter distinto */
  return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dst, const char *src) {
  int i = 0;
  while (src[i]) { /* copia carácter por carácter */
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0'; /* agrega el terminador */
  return dst;
}

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == (char)c)
      return (char *)s; /* encontrado */
    s++;
  }
  /* si buscamos '\0' devolvemos el final, sino NULL */
  return (c == '\0') ? (char *)s : 0;
}

void *memset(void *s, int c, int n) {
  unsigned char *p = (unsigned char *)s;
  while (n--) { /* llena n bytes */
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memcpy(void *dst, const void *src, int n) {
  unsigned char *d = (unsigned char *)dst;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) { /* copia n bytes */
    *d++ = *s++;
  }
  return dst;
}
