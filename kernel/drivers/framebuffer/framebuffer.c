#include "colors.h"
#include "stdint.h"

static uint32_t *fb; /* Framebuffer */

/* Aqui, la "G" significa que se supone que esto son variables globales */
uint32_t g_screen_width;
uint32_t g_screen_height;

/* Ahora si, aqui lo que usaremos, son static asi que otro archivo no las puede
 * llamar */
static uint32_t screen_width;
static uint32_t screen_height;
static uint32_t screen_pitch;

void video_init(uint32_t *framebuffer, uint32_t width, uint32_t height,
                uint32_t pitch) /* Esta funcion inicializa la pantalla*/
{
  /* Solo asignamos las variables que le pasaron a la funcion a las del que
   * usamos aqui en el driver */
  fb = framebuffer;
  screen_width = width;
  screen_height = height;
  screen_pitch = pitch;

  /* Asigmaos ahora las mismas variables pero ahora a las globales, asi otros
   * programas pueden saber esta informacion */
  g_screen_width = width;
  g_screen_height = height;
}

void video_put_pixel(int x, int y,
                     uint32_t color) /* Se te piden, una posicion en X, una
                                        posicion en Y, y un color en 32 bits*/
{
  if (x < 0 || y < 0 || x >= screen_width || y >= screen_height) {
    /* ¿Porque esto?, es porque si intentas poner un pixel en una posicion menor
     * a 0, esta fuera de la pantalla, lo mismo si es mas grande del tamaño de
     * la pantalla*/
    return;
  }
  fb[y * (screen_pitch / 4) + x] =
      color; /* Pone el pixel, del color que queremos, en la posicion que
                quermos */
}

void video_fill_screen(
    uint32_t color) /* Rellena la pantalla del color que le indiquemos */
{
  for (uint32_t y = 0; y < screen_height;
       y++) /* Mientas Y sea menor que el tamaño maximo permitido, aumenta la
               variable */
  {
    for (uint32_t x = 0; x < screen_width; x++) /* Lo mismo en el eje y */
    {
      video_put_pixel(
          x, y, color); /* Aqui esta lo de poner el pixel en la cordenada */
    }
  }
}

void video_fill_rect(int x, int y, int w, int h, uint32_t color) {
  /* X & Y son la cordenada de la esquina superior izquierda. w es width y h
   * height*/
  for (int iy = 0; iy < h; iy++) {
    for (int ix = 0; ix < w; ix++) {
      video_put_pixel(x + ix, y + iy, color);
    }
  }
}

uint32_t video_get_width() { return screen_width; }

uint32_t video_get_height() { return screen_height; }

void video_scroll(int lines) {
  int row_size = screen_pitch / 4;
  int rows_to_move = screen_height - lines;

  /* mover framebuffer hacia arriba */
  for (int y = 0; y < rows_to_move; y++) {
    for (int x = 0; x < screen_width; x++) {
      fb[y * row_size + x] = fb[(y + lines) * row_size + x];
    }
  }

  /* limpiar parte inferior */
  for (int y = rows_to_move; y < screen_height; y++) {
    for (int x = 0; x < screen_width; x++) {
      fb[y * row_size + x] = COLOR_BASE;
    }
  }
}
