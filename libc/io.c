#include <io.h>
#include <string.h>
#include <syscall.h>

int write(int fd, const char *buf, int len) {
  /* escribe len bytes de buf en el fd */
  return (int)syscall(SYS_WRITE, fd, (long)buf, len, 0, 0);
}

int print(const char *str) {
  /* imprime una cadena en stdout (fd 1) */
  return write(1, str, strlen(str));
}

int print_col(const char *str, uint32_t color) {
  /* imprime con color (llamada directa al driver de video) */
  return (int)syscall(SYS_PUTCHAR, (long)color, (long)str, strlen(str), 0, 0);
}

int read(int fd, char *buf, int len) {
  /* lee hasta len bytes del fd en buf */
  return (int)syscall(SYS_READ, fd, (long)buf, len, 0, 0);
}

void put_pixel(int x, int y, int rgb) {
  /* dibuja un píxel en (x, y) con color rgb */
  syscall(SYS_DRAW_PIXEL, x, y, rgb, 0, 0);
}

void clear(void) {
  /* limpia la pantalla */
  syscall(SYS_CLEAR, 0, 0, 0, 0, 0);
}

uint64_t get_cpu_cycles(void) {
  uint64_t ret;
  __asm__ volatile("rdtsc" : "=A"(ret));
  return ret;
}
