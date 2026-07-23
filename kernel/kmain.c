#include <arch/i686.h>
#include <boot/grub/multiboot.h>
#include <framebuffer.h>
#include <term.h>

#include <colors.h>
#include <stdint.h>

#include <kbd.h>

#include <gdt/gdt.h>
#include <interrupts.h>

#include <paging/paging.h>
#include <syscall/syscall.h>
#include <task/task.h>

#include <vfs.h>

void kmain(uint32_t magic, multiboot_info_t *bootinfo) {
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

  uint32_t fb_size = bootinfo->framebuffer_pitch * bootinfo->framebuffer_height;
  paging_init(bootinfo->mem_upper,
              (uint32_t)(uintptr_t)bootinfo->framebuffer_addr, fb_size);

  enable_interrupts();
  init_multitasking();
  vfs_init();

  const char *argv[] = {"/bin/sh", 0};
  spawn_elf("/bin/sh", 1, argv);

  for (;;) {
    halt();
  }
}
