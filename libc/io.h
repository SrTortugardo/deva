#ifndef IO_H
#define IO_H

#include <stdint.h>

#define FB_SCREEN_WIDTH 800
#define FB_SCREEN_HEIGHT 600

/* entrada/salida basica */
int print(const char *str);
int print_col(const char *str, uint32_t color);
int write(int fd, const char *buf, int len);
int read(int fd, char *buf, int len);
void put_pixel(int x, int y, int rgb);
void clear(void);
uint64_t get_cpu_cycles(void);

#endif
