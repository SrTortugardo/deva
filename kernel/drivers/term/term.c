#include <colors.h>
#include <fonts/font8x8.h>
#include <framebuffer.h>
#include <text.h>

static int cursor_x = 0;
static int cursor_y = 0;

void term_init() {
  /* Quizas luego ocupo hacer otra cosa */
  cursor_x = 0;
  cursor_y = 0;
}

void term_scroll_screen() /* Wrapper con cosas nuevas */ {
  video_scroll(FONT_HEIGHT + 1);
  if (cursor_y > 0) {
    cursor_y--;
  }
}
void term_check_scroll() {
  if ((cursor_y + 1) * (FONT_HEIGHT + 1) >= video_get_height()) {
    term_scroll_screen();
  }
}

void term_putchar(char c, uint32_t color) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
    term_check_scroll();
    return;
  }

  draw_char(c, cursor_x * FONT_WIDTH, cursor_y * FONT_HEIGHT, color);

  cursor_x++;

  if ((cursor_x + 1) * FONT_WIDTH >= video_get_width()) {
    cursor_x = 0;
    cursor_y++;
    term_check_scroll();
  }
}

void term_write(const char *str, uint32_t color) {
  while (*str) {
    term_putchar(*str++, color);
  }
}

void term_clear() {
  video_fill_screen(COLOR_BASE);

  cursor_x = 0;
  cursor_y = 0;
}

void kprint(const char *str) { term_write(str, COLOR_TEXT); }
