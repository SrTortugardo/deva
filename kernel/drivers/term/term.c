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

void term_scroll_screen() {
  video_scroll(FONT_HEIGHT +
               1); /* desplazamos el framebuffer una fila hacia arriba */
  if (cursor_y > 0) {
    cursor_y--; /* ajustamos el cursor a la nueva posicion */
  }
}
void term_check_scroll() {
  if ((cursor_y + 1) * (FONT_HEIGHT + 1) >= video_get_height()) {
    term_scroll_screen(); /* el cursor esta al pie de la pantalla, hacemos
                             scroll */
  }
}

void term_putchar(char c, uint32_t color) {
  if (c == '\n') {
    cursor_x = 0; /* volvemos al inicio de la linea */
    cursor_y++;   /* bajamos un renglon */
    term_check_scroll();
    return;
  }

  if (c == '\b') {
    if (cursor_x > 0) {
      cursor_x--;
      video_fill_rect(cursor_x * FONT_WIDTH, cursor_y * FONT_HEIGHT, FONT_WIDTH,
                      FONT_HEIGHT,
                      COLOR_BASE); /* borramos el caracter con color de fondo */
    }
    return;
  }

  draw_char(c, cursor_x * FONT_WIDTH, cursor_y * FONT_HEIGHT,
            color); /* dibujamos en la posicion actual */

  cursor_x++;

  if ((cursor_x + 1) * FONT_WIDTH >= video_get_width()) {
    cursor_x = 0; /* fin de linea: wrap */
    cursor_y++;
    term_check_scroll();
  }
}

void term_write(const char *str, uint32_t color) {
  while (*str) {
    term_putchar(*str++,
                 color); /* escribimos cada caracter hasta el null terminator */
  }
}

void term_clear() {
  video_fill_screen(COLOR_BASE);

  cursor_x = 0;
  cursor_y = 0;
}

void kprint(const char *str) { term_write(str, COLOR_TEXT); }
