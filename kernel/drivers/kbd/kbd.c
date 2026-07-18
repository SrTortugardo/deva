#include <colors.h>
#include <cpu.h>
#include <stdint.h>
#include <term.h>

/* Estado */
static int shift = 0;
static int extended = 0;

/* Keymaps */
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

  /* Scancode extendido */
  if (sc == 0xE0) {
    extended = 1;
    return;
  }

  /* Tecla liberada */
  if (sc & 0x80) {
    uint8_t key = sc & 0x7F;

    if (key == 0x2A || key == 0x36)
      shift = 0;

    return;
  }

  /* Ignorar teclas extendidas por ahora */
  if (extended) {
    extended = 0;
    return;
  }

  /* Shift presionado */
  if (sc == 0x2A || sc == 0x36) {
    shift = 1;
    return;
  }

  /* Convertir scancode a ASCII */
  if (sc < sizeof(kbd_map)) {
    uint8_t c;

    if (shift) {
      c = kbd_shift_map[sc];
    } else {
      c = kbd_map[sc];
    }

    if (c) {
      term_putchar(c, COLOR_TEXT);
    }
  }
}
