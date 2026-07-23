#include <syscall.h>

void *malloc(int size) {
  /* pide al kernel un bloque de memoria */
  return (void *)syscall(SYS_MALLOC, size, 0, 0, 0, 0);
}
void free(void *ptr) {
  /* libera el bloque que devolvió malloc */
  syscall(SYS_FREE, (long)ptr, 0, 0, 0, 0);
}
