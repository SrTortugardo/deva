#include <colors.h>
#include <io.h>
#include <proc.h>
#include <string.h>
#include <vfs.h>

#define MAX_ARGS 16
#define MAX_LINE 256

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
  char line[MAX_LINE]; /* buffer para la línea escrita */

  while (1) {                         /* bucle principal del shell */
    print_col("deva> ", COLOR_GREEN); /* mostrar prompt */

    int n = read(0, line, sizeof(line)); /* leer entrada del usuario */
    if (n <= 0)
      continue;
    if (line[0] == '\f') { /* Ctrl+L: limpiar y mostrar prompt */
      clear();
      continue;
    }

    char *tokens[MAX_ARGS]; /* array de tokens */
    int ntok = 0;
    char *p = line;

    while (*p && ntok < MAX_ARGS) { /* tokenizar la línea */
      while (*p == ' ')             /* saltar espacios */
        p++;
      if (!*p)
        break;

      if (*p == '"') { /* token entre comillas */
        p++;
        tokens[ntok] = p;
        while (*p && *p != '"')
          p++;
        if (*p) {
          *p = '\0'; /* cerrar comilla */
          p++;
        }
        ntok++;
      } else { /* token sin comillas */
        tokens[ntok] = p;
        while (*p && *p != ' ')
          p++;
        if (*p) {
          *p = '\0'; /* separar token */
          p++;
        }
        ntok++;
      }
    }

    if (ntok == 0) /* solo espacios en blanco */
      continue;

    char path[64];
    if (tokens[0][0] == '/') { /* ruta absoluta */
      int i = 0;
      while (tokens[0][i] && i < 63) {
        path[i] = tokens[0][i];
        i++;
      }
      path[i] = '\0';
    } else { /* prefijar /bin/ */
      int i = 0;
      path[0] = '/';
      path[1] = 'b';
      path[2] = 'i';
      path[3] = 'n';
      path[4] = '/';
      i = 5;
      int j = 0;
      while (tokens[0][j] && i < 63) {
        path[i] = tokens[0][j];
        i++;
        j++;
      }
      path[i] = '\0';
    }

    const char *argv_new[MAX_ARGS + 1]; /* preparar argv para spawn */
    argv_new[0] = path;
    for (int i = 1; i < ntok; i++)
      argv_new[i] = tokens[i];
    argv_new[ntok] = 0; /* terminador nulo */

    int pid = spawn(path, ntok, argv_new); /* crear proceso hijo */
    if (pid < 0) {
      print_col("comando no encontrado\n", COLOR_RED);
    } else {
      waitpid(pid); /* esperar a que termine */
    }
  }

  return 0;
}
