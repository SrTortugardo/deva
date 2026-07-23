#include <syscall.h>

void exit(int status) {
  /* mata el proceso actual con código status */
  syscall(SYS_EXIT, status, 0, 0, 0, 0);
  for (;;) { /* no debería llegar acá xd pero bueno, por si algo */
  }
}

int getpid(void) {
  /* pregunta al kernel nuestro PID */
  return (int)syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

void yield(void) {
  /* cede la CPU voluntariamente */
  syscall(SYS_YIELD, 0, 0, 0, 0, 0);
}

int spawn(const char *path, int argc, const char **argv) {
  /* crea un proceso hijo ejecutando path */
  return (int)syscall(SYS_SPAWN, (long)path, argc, (long)argv, 0, 0);
}

int waitpid(int pid) {
  /* espera a que el hijo pid termine */
  return (int)syscall(SYS_WAITPID, pid, 0, 0, 0, 0);
}
