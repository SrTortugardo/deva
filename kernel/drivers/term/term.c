#include <colors.h>
#include <kbd.h>
#include <term.h>
#include <tty_fb.h>

tty_t tty0;

static int input_get_key(key_event_t *ev) { return kbd_get_key(ev); }

void term_init() {
  static tty_input_t input;
  input.get_key = input_get_key;
  tty_init(&tty0, &tty_fb_output, &input);
}

void term_putchar(char c, uint32_t color) {
  tty0.fg = color;
  tty_putchar(&tty0, c);
}

void term_write(const char *str, uint32_t color) {
  tty0.fg = color;
  tty_write(&tty0, str);
}

void term_clear() { tty_clear(&tty0); }

void kprint(const char *str) {
  tty0.fg = COLOR_TEXT;
  tty_write(&tty0, str);
}
