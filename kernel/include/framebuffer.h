#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

void video_init(uint32_t *framebuffer, uint32_t w, uint32_t h, uint32_t pitch);

void video_put_pixel(int x, int y, uint32_t color);

void video_fill_rect(int x, int y, int w, int h, uint32_t color);

uint32_t video_get_width();
uint32_t video_get_height();

void video_scroll(int lines);

void video_fill_screen(uint32_t color);

#endif
