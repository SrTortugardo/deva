#ifndef TTY_H
#define TTY_H

#include <stdint.h>

typedef enum {
  KEY_NONE = 0,
  KEY_CHAR,
  KEY_BACKSPACE,
  KEY_ENTER,
  KEY_TAB,
  KEY_ESC,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_HOME,
  KEY_END,
  KEY_DELETE,
} key_type_t;

#define KEY_MOD_SHIFT 1
#define KEY_MOD_CTRL 2
#define KEY_MOD_ALT 4

typedef struct {
  key_type_t type;
  char c;
  uint8_t mods;
} key_event_t;

typedef struct {
  void (*draw_char)(int col, int row, char c, uint32_t fg, uint32_t bg);
  void (*scroll)(int rows);
  void (*clear)(void);
  int (*get_cols)(void);
  int (*get_rows)(void);
} tty_output_t;

typedef struct {
  int (*get_key)(key_event_t *ev);
} tty_input_t;

typedef struct tty {
  int cursor_x;
  int cursor_y;
  uint32_t fg;
  uint32_t bg;
  tty_output_t *output;
  tty_input_t *input;
} tty_t;

void tty_init(tty_t *tty, tty_output_t *output, tty_input_t *input);
void tty_putchar(tty_t *tty, char c);
void tty_write(tty_t *tty, const char *str);
void tty_clear(tty_t *tty);
int tty_readline(tty_t *tty, char *buf, int max);

#endif
