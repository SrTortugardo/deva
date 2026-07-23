#include <colors.h>
#include <io.h>
#include <stdbool.h>

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
  while (true) { /* bucle infinito */
    print("Hola mundo(tarea 2)\n");

    for (int i = 0; i < 500; i++) { /* espera ocupada larga */
      for (int j = 0; j < 10000; j++) {
        __asm__ volatile(""); /* evitar que el compilador optimice */
      }
    }
  }

  return 0;
}
