#include <io.h>

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
  clear(); /* limpiar la pantalla llamando al driver de video */
  return 0;
}
