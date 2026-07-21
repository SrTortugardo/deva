/*
 * Syscalls via int $0x80.
 * Por ahora solo existe SYS_TEST que imprime "syscall".
 */

#include <colors.h>
#include <stdint.h>
#include <syscall/syscall.h>
#include <term.h>

void syscall_handler_c(struct regs *r) {
  (void)r; /* Esta syscall no usa registros de argumentos */
  term_write("syscall\n", COLOR_CYAN);
}

uint32_t syscall_test(void) {
  uint32_t ret;
  __asm__ volatile("int $0x80"
                   : "=a"(ret)
                   : "a"((uint32_t)SYS_TEST)
                   : "memory");
  return ret;
}
