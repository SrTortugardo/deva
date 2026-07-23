#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* IRQ handler principal */
void keyboard_handler(void);

int kbd_pop(void);
int kbd_readline(char *buf, int max); /* leer linea terminada por \n */

#endif
