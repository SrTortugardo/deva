/*
 * Aqui van las funciones relacionadas a las idt's
 */

#include <klib/string.h>
#include <stddef.h>
#include <stdint.h>

struct idt_entry {
  uint16_t base_lo;
  uint16_t sel;
  uint8_t always0;
  uint8_t flags;
  uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

extern void idt_load(struct idt_ptr *idtptr);

/* ISR (exceptions) */
#define DECL_ISR(n) extern void isr##n(void)
DECL_ISR(0);
DECL_ISR(1);
DECL_ISR(2);
DECL_ISR(3);
DECL_ISR(4);
DECL_ISR(5);
DECL_ISR(6);
DECL_ISR(7);
DECL_ISR(8);
DECL_ISR(9);
DECL_ISR(10);
DECL_ISR(11);
DECL_ISR(12);
DECL_ISR(13);
DECL_ISR(14);
DECL_ISR(15);
DECL_ISR(16);
DECL_ISR(17);
DECL_ISR(18);
DECL_ISR(19);
DECL_ISR(20);
DECL_ISR(21);
DECL_ISR(22);
DECL_ISR(23);
DECL_ISR(24);
DECL_ISR(25);
DECL_ISR(26);
DECL_ISR(27);
DECL_ISR(28);
DECL_ISR(29);
DECL_ISR(30);
DECL_ISR(31);
#undef DECL_ISR

/* IRQ (hardware) */
extern void irq0_handler();
extern void irq1_handler();

static struct idt_entry idt[256];
static struct idt_ptr idtp;

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel,
                         uint8_t flags) {
  idt[num].base_lo = base & 0xFFFF;
  idt[num].base_hi = (base >> 16) & 0xFFFF;
  idt[num].sel = sel;
  idt[num].always0 = 0;
  idt[num].flags = flags;
}

extern void remap_pic(void); /* definido en irq.c */
/* pronto llegan las syscalls y isr 128 se usara */
// extern void isr128(void);

void idt_init(void) {
  memset(&idt, 0, sizeof(idt));

  remap_pic();

#define SET_ISR(n) idt_set_gate(n, (uint32_t)isr##n, 0x08, 0x8E)
  SET_ISR(0);
  SET_ISR(1);
  SET_ISR(2);
  SET_ISR(3);
  SET_ISR(4);
  SET_ISR(5);
  SET_ISR(6);
  SET_ISR(7);
  SET_ISR(8);
  SET_ISR(9);
  SET_ISR(10);
  SET_ISR(11);
  SET_ISR(12);
  SET_ISR(13);
  SET_ISR(14);
  SET_ISR(15);
  SET_ISR(16);
  SET_ISR(17);
  SET_ISR(18);
  SET_ISR(19);
  SET_ISR(20);
  SET_ISR(21);
  SET_ISR(22);
  SET_ISR(23);
  SET_ISR(24);
  SET_ISR(25);
  SET_ISR(26);
  SET_ISR(27);
  SET_ISR(28);
  SET_ISR(29);
  SET_ISR(30);
  SET_ISR(31);
#undef SET_ISR

  /* IRQs */
  idt_set_gate(32, (uint32_t)irq0_handler, 0x08, 0x8E);
  idt_set_gate(33, (uint32_t)irq1_handler, 0x08, 0x8E);
  // idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

  idtp.limit = sizeof(idt) - 1;
  idtp.base = (uint32_t)&idt;

  idt_load(&idtp);
}
