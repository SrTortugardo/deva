#include <cpu.h>
#include <drivers.h>
#include <stdint.h>
#include <tty.h>

static int shift = 0;
static int ctrl = 0;
static int extended = 0;

#define KBD_BUF_SIZE 256
static key_event_t kbd_buf[KBD_BUF_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

static void kbd_push_key(key_event_t ev) {
  int next = (kbd_head + 1) % KBD_BUF_SIZE;
  if (next == kbd_tail)
    return;
  kbd_buf[kbd_head] = ev;
  kbd_head = next;
}

int kbd_get_key(key_event_t *ev) {
  if (kbd_head == kbd_tail)
    return 0;
  *ev = kbd_buf[kbd_tail];
  kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
  return 1;
}

static const uint8_t kbd_map[58] = {
    0,   27,  '1',  '2',  '3',  '4', '5', '6',  '7', '8', '9', '0',
    '-', '=', '\b', '\t', 'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
    'o', 'p', '[',  ']',  '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
    'j', 'k', 'l',  ';',  '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm',  ',',  '.',  '/', 0,   '*',  0,   ' '};

static const uint8_t kbd_shift_map[58] = {
    0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
    '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
    'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
    'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' '};

void keyboard_handler(void) {
  uint8_t sc = inb(0x60);

  if (sc == 0xE0) {
    extended = 1;
    return;
  }

  if (sc & 0x80) {
    uint8_t key = sc & 0x7F;

    if (key == 0x2A || key == 0x36)
      shift = 0;
    if (key == 0x1D)
      ctrl = 0;

    return;
  }

  if (extended) {
    extended = 0;
    return;
  }

  if (sc == 0x2A || sc == 0x36) {
    shift = 1;
    return;
  }

  if (sc == 0x1D) {
    ctrl = 1;
    return;
  }

  if (sc < sizeof(kbd_map)) {
    uint8_t c;

    if (shift)
      c = kbd_shift_map[sc];
    else
      c = kbd_map[sc];

    if (ctrl) {
      if (c >= 'a' && c <= 'z')
        c = c - 'a' + 1;
      else if (c >= 'A' && c <= 'Z')
        c = c - 'A' + 1;
    }

    if (c) {
      key_event_t ev;
      ev.mods = 0;
      if (shift)
        ev.mods |= KEY_MOD_SHIFT;
      if (ctrl)
        ev.mods |= KEY_MOD_CTRL;

      if (c == '\b') {
        ev.type = KEY_BACKSPACE;
        ev.c = c;
      } else if (c == '\n') {
        ev.type = KEY_ENTER;
        ev.c = c;
      } else if (c == '\t') {
        ev.type = KEY_TAB;
        ev.c = c;
      } else if (c == 27) {
        ev.type = KEY_ESC;
        ev.c = c;
      } else {
        ev.type = KEY_CHAR;
        ev.c = (char)c;
      }
      kbd_push_key(ev);
    }
  }
}

static int keyboard_init(void) { return 0; }

static struct driver keyboard = {.name = "Teclado PS/2",
                                 .author = "SrTortugardo",
                                 .version = 1,
                                 .init = keyboard_init};

void keyboard_register() { driver_register(&keyboard); }
