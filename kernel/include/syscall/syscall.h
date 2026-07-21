#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

struct regs; /* forward declaration, definido en interrupts.h */

enum {
  SYS_TEST,
};

uint32_t syscall_test(void); /* quizas no sea la mejor idea tener esto aqui */

/* Handler C invocado desde isr128. */
void syscall_handler_c(struct regs *r);

#endif
