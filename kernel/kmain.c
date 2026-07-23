#include <kernel.h>

#include <arch/i686.h>
#include <boot/grub/multiboot.h>
#include <framebuffer.h>
#include <term.h>

#include <colors.h>
#include <stdint.h>

/* Drivers */
#include <ata.h>
#include <drivers.h>
#include <kbd.h>

#include <gdt/gdt.h>
#include <interrupts.h>

#include <paging/paging.h>
#include <syscall/syscall.h>
#include <task/task.h>

#include <vfs.h>

/* Alguien debia conocer todos los drivers */
void drivers_register_all() {
  keyboard_register();
  ata_register();
}

void init_stage1(uint32_t magic, multiboot_info_t *bootinfo) {
  video_init((uint32_t *)(uintptr_t)bootinfo->framebuffer_addr,
             bootinfo->framebuffer_width, bootinfo->framebuffer_height,
             bootinfo->framebuffer_pitch);

  init_gdt();

  idt_init();

  pit_init(100); /* 100 Hz es lo comun */
  term_init();
}

void init_stage2(multiboot_info_t *bootinfo) {
  irq_install_handler(1, keyboard_handler);

  irq_unmask(); /* desenmascarar IRQ0 (timer) e IRQ1 (teclado) en el PIC */

  uint32_t fb_size = bootinfo->framebuffer_pitch * bootinfo->framebuffer_height;
  paging_init(bootinfo->mem_upper,
              (uint32_t)(uintptr_t)bootinfo->framebuffer_addr, fb_size);

  enable_interrupts();
  init_multitasking();
  vfs_init();
}

void init_stage3() {
  /* Aqui quiero hacer cosas en un futuro */
  kprint("Bienvenido Seas A... ");
  term_write("Deva", COLOR_PINK);
  term_write("(i686)\n", COLOR_YELLOW);
  kprint("\n\n");
}

void kmain(uint32_t magic, multiboot_info_t *bootinfo) {
  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    for (;;) {
      halt();
    }
  }

  init_stage1(magic, bootinfo);

  drivers_register_all();
  drivers_init();

  init_stage2(bootinfo);
  init_stage3();

  const char *argv[] = {"/bin/sh", 0};
  spawn_elf("/bin/sh", 1, argv);

  for (;;) {
    halt();
  }
}
