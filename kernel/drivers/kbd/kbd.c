#include <arch/i686.h>
#include <colors.h>
#include <cpu.h>
#include <stdint.h>
#include <term.h>

/* Estado */
static int shift = 0;
static int ctrl = 0;
static int extended = 0;

/* BUFFER CIRCULAR */
#define KBD_BUF_SIZE 256
static char kbd_buf[KBD_BUF_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

static void kbd_push(char c) {
  int next = (kbd_head + 1) % KBD_BUF_SIZE; /* siguiente posicion circular */
  if (next == kbd_tail)
    return; /* buffer lleno, descartamos el caracter */

  kbd_buf[kbd_head] = c;
  kbd_head = next;
}

/* Saca el proximo char o -1 si vacio. */
int kbd_pop(void) {
  if (kbd_head == kbd_tail)
    return -1; /* buffer vacio */
  char c = kbd_buf[kbd_tail];
  kbd_tail =
      (kbd_tail + 1) % KBD_BUF_SIZE; /* avanzamos la cola circularmente */
  return (int)(unsigned char)c;
}

int kbd_readline(char *buf, int max) {
  int n = 0;
  if (max <= 0)
    return 0;

  enable_interrupts();

  for (;;) {
    int c = kbd_pop();
    if (c < 0) {
      halt(); /* no hay tecla, detenemos el CPU hasta la proxima interrupcion */
      continue;
    }

    if (c == '\n') {
      term_putchar('\n', COLOR_TEXT);
      break;
    }
    if (c == '\f') {
      buf[0] = '\f'; /* Ctrl+L: devolver \f al usuario para que la shell */
      buf[1] = '\0'; /* ejecute /bin/clear y redibuje el prompt         */
      return 1;
    }
    if (c == '\b') {
      if (n > 0) {
        n--;
        term_putchar('\b', COLOR_TEXT); /* mover cursor atras */
        term_putchar(' ', COLOR_TEXT);  /* pintar espacio para borrar */
        term_putchar('\b', COLOR_TEXT); /* mover cursor atras de nuevo */
      }
      continue;
    }

    if (n < max - 1) { /* dejamos espacio para el null terminator */
      buf[n++] = (char)c;
      term_putchar((char)c, COLOR_TEXT);
    }
  }

  buf[n] = '\0';
  return n;
}

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
  uint8_t sc = inb(0x60); /* leemos el scancode del puerto del teclado */

  if (sc == 0xE0) { /* prefijo de tecla extendida (flechas, home, etc) */
    extended = 1;
    return;
  }

  if (sc & 0x80) {           /* bit 7 = 1 indica que se solto la tecla */
    uint8_t key = sc & 0x7F; /* scancode base sin el bit de release */

    if (key == 0x2A || key == 0x36) /* shift izquierdo o derecho liberado */
      shift = 0;
    if (key == 0x1D) /* control liberado */
      ctrl = 0;

    return;
  }

  if (extended) { /* ignoramos teclas extendidas por ahora */
    extended = 0;
    return;
  }

  if (sc == 0x2A || sc == 0x36) { /* shift izquierdo o derecho presionado */
    shift = 1;
    return;
  }

  if (sc == 0x1D) { /* control presionado */
    ctrl = 1;
    return;
  }

  if (sc < sizeof(kbd_map)) { /* el scancode esta dentro del keymap */
    uint8_t c;

    if (shift) {
      c = kbd_shift_map[sc]; /* keymap con shift: mayusculas y simbolos */
    } else {
      c = kbd_map[sc]; /* keymap normal: minusculas y numeros */
    }

    if (ctrl) {
      if (c >= 'a' && c <= 'z')
        c = c - 'a' + 1; /* Ctrl+a → 1 (SOH), …, Ctrl+z → 26 (SUB) */
      else if (c >= 'A' && c <= 'Z')
        c = c - 'A' + 1; /* Ctrl+Shift+a → 1, …, Ctrl+Shift+z → 26 */
    }
    if (c) {
      kbd_push((char)c); /* encolamos el caracter en el buffer circular */
    }
  }
}
