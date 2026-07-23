#include <colors.h>
#include <fonts/font8x8.h>
#include <framebuffer.h>
#include <text.h>
#include <tty_fb.h>

#define ROW_HEIGHT (FONT_HEIGHT + 1)

static void fb_draw_char(int col, int row, char c, uint32_t fg, uint32_t bg) {
  int x = col * FONT_WIDTH;
  int y = row * FONT_HEIGHT;
  video_fill_rect(x, y, FONT_WIDTH, FONT_HEIGHT, bg);
  draw_char(c, x, y, fg);
}

static void fb_scroll(int rows) {
  for (int i = 0; i < rows; i++)
    video_scroll(ROW_HEIGHT);
}

static void fb_clear(void) { video_fill_screen(COLOR_BASE); }

static int fb_get_cols(void) { return video_get_width() / FONT_WIDTH; }

static int fb_get_rows(void) { return video_get_height() / ROW_HEIGHT; }

tty_output_t tty_fb_output = {
    .draw_char = fb_draw_char,
    .scroll = fb_scroll,
    .clear = fb_clear,
    .get_cols = fb_get_cols,
    .get_rows = fb_get_rows,
};
