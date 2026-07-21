#include <arch/i686.h>
#include <boot/grub/multiboot.h>
#include <framebuffer.h>
#include <term.h>

#include <colors.h>
#include <stdint.h>

#include <kbd.h>

#include <gdt/gdt.h>
#include <interrupts.h>

#include <syscall/syscall.h>
#include <task/task.h>

#include <vfs.h>

static void task_a(void) {
  /* ŧarea de prueba */
  for (;;) {
    syscall_test();
    for (volatile int i = 0; i < 200000; i++)
      ;
  }
}

static void task_b(void) {
  for (;;) {
    term_write("B", COLOR_RED);
    for (volatile int i = 0; i < 200000; i++)
      ;
  }
}

static void task_c(void) {
  for (;;) {
    term_write("C", COLOR_YELLOW);
    for (volatile int i = 0; i < 200000; i++)
      ;
  }
}

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

  enable_interrupts();
  init_multitasking();
  vfs_init();

  /* ya aqui hacemos lo que queremos */
  term_write("Paralilepipedo\n", COLOR_MAGENTA);
  char buffer[VFS_READ_BUFFER_SIZE];
  int len = vfs_read("/etc/LOBO.TXT", buffer, VFS_READ_BUFFER_SIZE);
  if (len > 0) {
    buffer[len] = '\0';
    term_write(buffer, COLOR_TEXT);
  }

  create_process(task_a, "task_a");
  create_process(task_b, "task_b");
  create_process(task_c, "task_c");

  for (;;) {
    halt();
  }
}
