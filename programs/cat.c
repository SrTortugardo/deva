#include <io.h>
#include <mem.h>
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

int main(int argc, char **argv) {
  if (argc < 2) /* falta el nombre del archivo */
    return 1;

  vfs_stat_t st;
  if (vfs_stat(argv[1], &st) < 0) /* obtener tamaño del archivo */
    return 1;

  char *buf = malloc(st.size + 1); /* +1 para el terminador nulo */
  if (!buf)
    return 1;

  int n = vfs_read(argv[1], buf, st.size); /* leer todo el archivo */
  if (n < 0) {
    free(buf);
    return 1;
  }

  buf[n] = '\0'; /* asegurar que sea una cadena */
  print(buf);    /* volcar contenido a pantalla */
  free(buf);
  return 0;
}
