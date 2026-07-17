#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

void draw_char(char c, int x, int y, uint32_t color);

void draw_string(const char *str, int x, int y, uint32_t color);

#endif
