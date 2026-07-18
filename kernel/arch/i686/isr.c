#include <colors.h>
#include <interrupts.h>
#include <stdint.h>
#include <term.h>

static const char *exception_names[32] = {"Divide Error",
                                          "Debug",
                                          "NMI",
                                          "Breakpoint",
                                          "Overflow",
                                          "Bound Range Exceeded",
                                          "Invalid Opcode",
                                          "Device Not Available",
                                          "Double Fault",
                                          "Coprocessor Segment Overrun",
                                          "Invalid TSS",
                                          "Segment Not Present",
                                          "Stack Fault",
                                          "General Protection Fault",
                                          "Page Fault",
                                          "Reserved",
                                          "FPU Exception",
                                          "Alignment Check",
                                          "Machine Check",
                                          "SIMD FP Exception",
                                          "Virtualization",
                                          "Control Protection",
                                          "Reserved",
                                          "Reserved",
                                          "Reserved",
                                          "Reserved",
                                          "Reserved",
                                          "Reserved",
                                          "Hypervisor Injection",
                                          "VMM Communication",
                                          "Security",
                                          "Reserved"};

static void term_put_hex(uint32_t val) {
  static const char hex[] = "0123456789ABCDEF";
  char buf[11];
  buf[0] = '0';
  buf[1] = 'x';
  for (int i = 0; i < 8; i++)
    buf[2 + i] = hex[(val >> (28 - i * 4)) & 0xF];
  buf[10] = '\0';
  term_write(buf, COLOR_RED);
}

void exception_handler_c(struct regs *r) {
  term_write("\n[EXCEPCION] ", COLOR_RED);

  if (r->int_no < 32)
    term_write(exception_names[r->int_no], COLOR_RED);
  else
    term_write("Ocurrio una excepcion desconocida?", COLOR_RED);

  term_write("\n int_no=", COLOR_RED);
  term_put_hex(r->int_no);
  term_write(" err_code=", COLOR_RED);
  term_put_hex(r->err_code);
  term_write("\n eip=", COLOR_RED);
  term_put_hex(r->eip);
  term_write(" cs=", COLOR_RED);
  term_put_hex(r->cs);
  term_write(" eflags=", COLOR_RED);
  term_put_hex(r->eflags);
  term_write("\n eax=", COLOR_RED);
  term_put_hex(r->eax);
  term_write(" ebx=", COLOR_RED);
  term_put_hex(r->ebx);

  while (1)
    __asm__ volatile("cli; hlt");
}
