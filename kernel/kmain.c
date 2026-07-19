#include <arch/i686.h>
#include <boot/grub/multiboot.h>
#include <framebuffer.h>
#include <term.h>

#include <colors.h>
#include <stdint.h>

#include <kbd.h>

#include <gdt/gdt.h>
#include <interrupts.h>

void kmain(uint32_t magic, multiboot_info_t *bootinfo) {
  /* Si no entramos por un bootloader multiboot valido, bootinfo es basura. */
  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    for (;;) {
      halt();
    }
  }

  video_init((uint32_t *)(uintptr_t)bootinfo->framebuffer_addr,
             bootinfo->framebuffer_width, bootinfo->framebuffer_height,
             bootinfo->framebuffer_pitch);

  init_gdt();

  idt_init();

  pit_init(100); /* 100 Hz es lo comun */

  irq_install_handler(1, keyboard_handler);

  irq_unmask(); /* desenmascarar IRQ0 (timer) e IRQ1 (teclado) en el PIC */
  term_init();
  term_write("Paralilepipedo\n", COLOR_MAGENTA);

  enable_interrupts();

  for (;;) {
    halt();
  }
}
