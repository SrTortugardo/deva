#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

struct regs {
  uint32_t gs, fs, es, ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no, err_code;
  uint32_t eip, cs, eflags, useresp, ss;
};

void idt_init(void);
void pit_init(uint32_t freq);

void irq_install_handler(int irq, void (*handler)(void));
void irq_unmask(void);
void irq_enable_keyboard(void);

#endif
