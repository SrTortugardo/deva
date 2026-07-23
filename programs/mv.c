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
  if (argc < 3) /* se necesitan origen y destino */
    return 1;

  vfs_stat_t st;
  if (vfs_stat(argv[1], &st) < 0) /* obtener tamaño del origen */
    return 1;

  char *buf = malloc(st.size); /* buffer temporal */
  if (!buf)
    return 1;

  int n = vfs_read(argv[1], buf, st.size); /* leer archivo fuente */
  if (n < 0) {
    free(buf);
    return 1;
  }

  if (vfs_write(argv[2], buf, n) < 0) { /* escribir en el destino */
    free(buf);
    return 1;
  }

  free(buf);
  vfs_delete(argv[1]); /* eliminar el archivo original */
  return 0;
}
