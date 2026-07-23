#ifndef IO_H
#define IO_H

#include <stdint.h>

/* colores abreviados para la terminal grafica */
#define COL_BASE     0x000000
#define COL_SURFACE  0x555555
#define COL_TEXT     0xFFFFFF
#define COL_RED      0xAA0000
#define COL_GREEN    0x00AA00
#define COL_BLUE     0x0000AA
#define COL_YELLOW   0xFFFF55
#define COL_CYAN     0x00AAAA
#define COL_MAGENTA  0xAA00AA
#define COL_GRAY     0xAAAAAA
#define COL_ORANGE   0xAA5500
#define COL_PINK     0xFF55FF

/* entrada/salida basica */
int print(const char *str);
int print_col(const char *str, uint32_t color);
int write(int fd, const char *buf, int len);
int read(int fd, char *buf, int len);
void put_pixel(int x, int y, int rgb);
void clear(void);

#endif
