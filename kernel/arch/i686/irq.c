#include <cpu.h>
#include <interrupts.h>
#include <stdint.h>
#include <task/task.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

#define PIT_FREQUENCY 1193180

static inline void send_eoi(int irq) {
  if (irq >= 8) {
    outb(0xA0, 0x20); /* esclavo */
  }
  outb(0x20, 0x20); /* maestro */
}

void remap_pic(void) {
  /* Inicializar PIC pero no habilitar IRQs todavia */
  outb(PIC1, 0x11);
  outb(PIC2, 0x11);
  outb(PIC1 + 1, 0x20);
  outb(PIC2 + 1, 0x28);
  outb(PIC1 + 1, 0x04);
  outb(PIC2 + 1, 0x02);
  outb(PIC1 + 1, 0x01);
  outb(PIC2 + 1, 0x01);

  /* Mask todas las IRQs inicialmente */
  outb(PIC1 + 1, 0xFF);
  outb(PIC2 + 1, 0xFF);
}

void pit_init(uint32_t freq) {
  uint16_t divisor = (uint16_t)(PIT_FREQUENCY / freq);

  outb(PIT_COMMAND, 0x36);
  outb(PIT_CHANNEL0, divisor & 0xFF);
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}

void irq_unmask(void) {
  uint8_t master = 0xFC; /* Habilitar IRQ0 (timer) y IRQ1 (keyboard) */
  uint8_t slave = 0xFF;  /* Mask todos los del slave */

  outb(0x21, master);
  outb(0xA1, slave);
}

void irq_enable_keyboard(void) {
  uint8_t master = inb(0x21);
  master &= ~(1 << 1);
  outb(0x21, master);
}

static void (*irq_routines[16])(void) = {0};

void irq_install_handler(int irq, void (*handler)(void)) {
  if (irq >= 0 && irq < 16)
    irq_routines[irq] = handler;
}

volatile uint32_t timer_ticks = 0;

void irq_handler_c(struct regs *r) {
  int irq = r->int_no - 32;

  /* EOI temprano: como schedule() puede hacer context_switch a una tarea
   * nueva que arrancara con sti, necesitamos que el PIC ya no este
   * asserting IRQ0; si no, el sti re-dispararia el timer al instante. */
  if (r->int_no >= 40)
    outb(0xA0, 0x20);
  outb(0x20, 0x20);

  if (irq == 0) {
    timer_ticks++;
    /* Multitarea preemptiva: en cada tick del timer se cambia de tarea. */
    schedule();
  }
  /* Keyboard - llama handler */
  else if (irq == 1) {
    if (irq_routines[1])
      irq_routines[1]();
  }
}

void enable_interrupts(void) { asm volatile("sti"); }
