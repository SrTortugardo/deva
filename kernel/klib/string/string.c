#include <stdbool.h>
#include <stddef.h>

int strlen(const char *s) {
  int ch, len;
  ch = 0;
  len = 0;

  while (s[ch++] != 0) {
    len++;
  }
  return len;
}

int strcmp(char *s1, char *s2) {
  int len1, len2, diferences, ch;
  diferences = 0;
  len1 = strlen(s1);
  len2 = strlen(s2);
  if (len1 != len2)
    return 1;

  for (ch = 0; ch < len1; ch++)
    diferences += (s1[ch] != s2[ch]);
  return diferences;
}

char *strcpy(char *dest, const char *src) {
  int i = 0; /* se empieza en el caracter 0 */

  while (src[i] != '\0') { /* \0 es el final */
    dest[i] = src[i];      /* copear el caracter */
    i++;
  }

  dest[i] = '\0'; /* poner el \0 */
  return dest;
}

char *strcat(char *dest, char *src) {
  char *start = dest;

  while (*dest != '\0') { /* ir al final */
    dest++;
  }

  strcpy(dest, src); /* agregar */

  return start;
}

void itoa(int num, char *str) {
  int i = 0;
  bool negative = false;

  if (num == 0) {
    str[0] = '0';
    str[1] = '\0';
    return;
  }

  if (num < 0) {
    negative = true;
    num = -num; /* hacer el numero positivo */
  }

  while (num > 0) {
    str[i++] = (num % 10) + 48; /* 48 es 0 en ascii */
    num = num / 10;
  }

  if (negative) {
    str[i++] = '-'; /* Agregar al final un - */
  }

  str[i] = '\0';

  int start = 0;
  char tmp;
  i--; /* para no estar en \0 */
  /* invertir */
  while (start < i) {
    /* i es el final*/
    tmp = str[start];
    str[start] = str[i];
    str[i] = tmp;

    start++;
    i--;
  }
}

int atoi(const char *str) { /* funcion robada */
  int res = 0;
  int sign = 1;

  while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
    str++;
  }

  if (*str == '-') {
    sign = -1;
    str++;
  } else if (*str == '+') {
    str++;
  }

  while (*str >= '0' && *str <= '9') {
    res = res * 10 + (*str - '0');
    str++;
  }

  return sign * res;
}

/* Cosas relacionadas a la memoria*/
void *
memcpy(void *dest, const void *src,
       size_t n) /* Copear memoria sin seguridad siempre usa proteccion <3 */
{
  uint8_t *d = dest;
  const uint8_t *s = src;
  while (n--)
    *d++ = *s++;
  return dest;
}

void *memmove(void *dest, const void *src,
              size_t n) /* Ahora si usamos proteccion(creo) */
{
  uint8_t *d = dest;
  const uint8_t *s = src;

  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }

  return dest;
}

void *memset(void *s, int c,
             size_t n) /* Llenar la memoria con el valor que se te antoje */
{
  uint8_t *p = s;
  while (n--)
    *p++ = (uint8_t)c;
  return s;
}

int memcmp(const void *s1, const void *s2,
           size_t n) /* Comparar mem byte por byte */
{
  const uint8_t *p1 = s1;
  const uint8_t *p2 = s2;

  while (n--) {
    if (*p1 != *p2)
      return *p1 - *p2;
    p1++;
    p2++;
  }

  return 0;
}

void *memchr(const void *s, int c, size_t n) /* Buscar byte en mem */
{
  const uint8_t *p = s;
  while (n--) {
    if (*p == (uint8_t)c)
      return (void *)p;
    p++;
  }
  return NULL;
}
