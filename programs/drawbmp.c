#include <io.h>
#include <mem.h>
#include <stdint.h>
#include <string.h>
#include <vfs.h>

__attribute__((naked)) void _start(void) {
  __asm__ volatile("mov 0(%%esp), %%ecx\n\t"
                   "lea 4(%%esp), %%edx\n\t"
                   "push %%edx\n\t"
                   "push %%ecx\n\t"
                   "call main\n\t"
                   "push %%eax\n\t"
                   "call exit\n\t"
                   "1: hlt\n\t"
                   "jmp 1b"
                   :
                   :
                   : "ecx", "edx");
}

static uint32_t rd32(uint8_t *p) { /* leer entero little-endian de 32 bits */
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}

static int32_t rd32s(uint8_t *p) { return (int32_t)rd32(p); }
static uint32_t rd16(uint8_t *p) { /* leer entero little-endian de 16 bits */
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

int main(int argc, char **argv) {
  if (argc < 2) { /* falta la ruta del archivo */
    print("uso: drawbmp <archivo.bmp>\n");
    return 1;
  }

  char *path = argv[1];
  vfs_stat_t st;

  if (vfs_stat(path, &st) < 0) { /* comprobar que el archivo existe */
    print("no existe\n");
    return 1;
  }

  uint8_t header[64]; /* cabecera BMP fija (54 bytes) */

  int n = vfs_read(path, (char *)header, 64);
  if (n < 64 || header[0] != 'B' ||
      header[1] != 'M') { /* validar magic number */
    print("bmp invalido\n");
    return 1;
  }

  uint32_t data_off = rd32(header + 10);   /* offset al inicio de los píxeles */
  int32_t width = rd32s(header + 18);      /* ancho en píxeles */
  int32_t height_raw = rd32s(header + 22); /* alto (signo indica orientación) */
  uint32_t bpp = rd16(header + 28);        /* bits por píxel */

  if (bpp != 8) { /* solo soportamos 8 bpp (paleta) */
    print("bppl\n");
    return 1;
  }

  int top_down = (height_raw < 0); /* BMP puede venir invertido */
  int32_t height = top_down ? -height_raw : height_raw;
  uint32_t row_size = (width + 3) & ~3u; /* cada fila se alinea a 4 bytes */

  if (row_size > 4096) { /* límite de seguridad */
    print("muy ancho\n");
    return 1;
  }

  uint8_t palette[1024];             /* paleta de 256 colores × 4 bytes */
  uint32_t pal_size = data_off - 54; /* tamaño de la paleta */

  if (pal_size > 1024)
    pal_size = 1024;

  if (vfs_pread(path, (char *)palette, pal_size, 54) < (int)pal_size) {
    print("bmp invalido\n");
    return 1;
  }

  uint32_t origin_x = (800 - (uint32_t)width) / 2; /* centrar en 800×600 */
  uint32_t origin_y = (600 - (uint32_t)height) / 2;
  uint8_t row_buf[4096]; /* buffer para una fila de píxeles */

  for (int32_t row = 0; row < height; row++) { /* recorrer filas */
    int32_t src_row =
        top_down ? row : (height - 1 - row); /* BMP está al revés */
    uint32_t offset = data_off + (uint32_t)src_row * row_size;

    if (vfs_pread(path, (char *)row_buf, row_size, offset) < (int)row_size)
      break; /* error de lectura, abortar */

    for (int32_t col = 0; col < width; col++) { /* recorrer columnas */
      uint8_t idx = row_buf[col];
      uint32_t rgb = ((uint32_t)palette[idx * 4 + 2] << 16) | /* BGR → RGB */
                     ((uint32_t)palette[idx * 4 + 1] << 8) | palette[idx * 4];

      put_pixel((int)(origin_x + (uint32_t)col), /* dibujar píxel */
                (int)(origin_y + (uint32_t)row), (int)rgb);
    }
  }

  return 0;
}
