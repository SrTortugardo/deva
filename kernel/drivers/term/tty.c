#include <tty.h>

void tty_init(tty_t *tty, tty_output_t *output, tty_input_t *input) {
  tty->cursor_x = 0;
  tty->cursor_y = 0;
  tty->fg = 0xFFFFFF;
  tty->bg = 0x000000;
  tty->output = output;
  tty->input = input;
}

static void tty_scroll(tty_t *tty) {
  tty->output->scroll(1);
  if (tty->cursor_y > 0)
    tty->cursor_y--;
}

static void tty_check_scroll(tty_t *tty) {
  if (tty->cursor_y >= tty->output->get_rows()) {
    tty_scroll(tty);
  }
}

void tty_putchar(tty_t *tty, char c) {
  if (c == '\n') {
    tty->cursor_x = 0;
    tty->cursor_y++;
    tty_check_scroll(tty);
    return;
  }

  if (c == '\r') {
    tty->cursor_x = 0;
    return;
  }

  if (c == '\b') {
    if (tty->cursor_x > 0) {
      tty->cursor_x--;
      tty->output->draw_char(tty->cursor_x, tty->cursor_y, ' ', tty->fg,
                             tty->bg);
    }
    return;
  }

  if (c == '\t') {
    int next = (tty->cursor_x / 4 + 1) * 4;
    int cols = tty->output->get_cols();
    while (tty->cursor_x < next && tty->cursor_x < cols) {
      tty->output->draw_char(tty->cursor_x, tty->cursor_y, ' ', tty->fg,
                             tty->bg);
      tty->cursor_x++;
    }
    if (tty->cursor_x >= cols) {
      tty->cursor_x = 0;
      tty->cursor_y++;
      tty_check_scroll(tty);
    }
    return;
  }

  tty->output->draw_char(tty->cursor_x, tty->cursor_y, c, tty->fg, tty->bg);
  tty->cursor_x++;

  if (tty->cursor_x >= tty->output->get_cols()) {
    tty->cursor_x = 0;
    tty->cursor_y++;
    tty_check_scroll(tty);
  }
}

void tty_write(tty_t *tty, const char *str) {
  while (*str) {
    tty_putchar(tty, *str++);
  }
}

void tty_clear(tty_t *tty) {
  tty->output->clear();
  tty->cursor_x = 0;
  tty->cursor_y = 0;
}

int tty_readline(tty_t *tty, char *buf, int max) {
  int n = 0;
  if (max <= 0)
    return 0;

  for (;;) {
    key_event_t ev;
    while (!tty->input->get_key(&ev)) {
      __asm__ volatile("hlt");
    }

    if (ev.type == KEY_ENTER) {
      tty_putchar(tty, '\n');
      break;
    }

    if (ev.type == KEY_CHAR && ev.c == '\f') {
      buf[0] = '\f';
      buf[1] = '\0';
      return 1;
    }

    if (ev.type == KEY_BACKSPACE) {
      if (n > 0) {
        n--;
        tty_putchar(tty, '\b');
      }
      continue;
    }

    if (ev.type == KEY_CHAR && n < max - 1) {
      buf[n++] = ev.c;
      tty_putchar(tty, ev.c);
      continue;
    }
  }

  buf[n] = '\0';
  return n;
}
