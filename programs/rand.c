#include <io.h>
#include <stdlib.h>

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
  int r = rand();
  char buf[16];
  int i = 0;
  int neg = 0;

  if (r < 0) {
    neg = 1;
    r = -r;
  }

  do {
    buf[i++] = (r % 10) + '0';
    r /= 10;
  } while (r > 0);

  if (neg)
    buf[i++] = '-';

  for (int j = 0; j < i / 2; j++) {
    char t = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = t;
  }
  buf[i] = '\n';
  write(1, buf, i + 1);

  return 0;
}
