#include <io.h>
#include <stdint.h>
#include <syscall.h>

/* Valor absoluto de i */
int abs(int i) { return i < 0 ? -i : i; }

/* Memoria */
void *malloc(int size) {
  /* pide al kernel un bloque de memoria */
  return (void *)syscall(SYS_MALLOC, size, 0, 0, 0, 0);
}
void free(void *ptr) {
  /* libera el bloque que devolvió malloc */
  syscall(SYS_FREE, (long)ptr, 0, 0, 0, 0);
}

/* numeros aleatorios */
int srand(int seed) {
  /* Siempre havia querido hacer esta funcion */
  seed = seed * 1664525 + 1013904223;
  return seed;
}

int rand() {
  uint64_t seed = get_cpu_cycles();
  srand(seed);
}
