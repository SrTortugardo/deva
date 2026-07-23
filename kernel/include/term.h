#ifndef TERM_H
#define TERM_H

#include <stdint.h>
#include <tty.h>

extern tty_t tty0;

void term_init(void);
void term_clear(void);
void term_putchar(char c, uint32_t color);
void term_write(const char *str, uint32_t color);
void kprint(const char *str);

#endif
