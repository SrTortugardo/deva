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
  if (argc < 3) /* se necesitan texto y archivo destino */
    return 1;

  int len = 0;
  for (int i = 1; i < argc - 1; i++) { /* calcular largo total del texto */
    if (i > 1)
      len++; /* espacio entre palabras */
    len += strlen(argv[i]);
  }

  char *content = malloc(len + 1); /* buffer para el contenido completo */
  if (!content)
    return 1;

  int pos = 0;
  for (int i = 1; i < argc - 1; i++) { /* concatenar argumentos con espacios */
    if (i > 1)
      content[pos++] = ' ';
    int sl = strlen(argv[i]);
    memcpy(content + pos, argv[i], sl);
    pos += sl;
  }
  content[pos] = '\0'; /* terminar la cadena */

  int ret = vfs_write(argv[argc - 1], content, pos); /* escribir al archivo */
  free(content);
  return ret < 0 ? 1 : 0;
}
