#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <tty.h>

void keyboard_handler(void);

int kbd_get_key(key_event_t *ev);

void keyboard_register();

#endif
