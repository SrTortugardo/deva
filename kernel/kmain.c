#include <arch/i686.h>
#include <boot/grub/multiboot.h>
#include <framebuffer.h>
#include <term.h>

#include <colors.h>
#include <stdint.h>

void kmain(uint32_t magic, multiboot_info_t *bootinfo) {
  video_init((uint32_t *)(uintptr_t)bootinfo->framebuffer_addr,
             bootinfo->framebuffer_width, bootinfo->framebuffer_height,
             bootinfo->framebuffer_pitch);

  term_init();
  term_write("Paralilepipedo\n", COLOR_MAGENTA);
  kprint("Esternocleidomastoideo\n");
  kprint("Manzana verde\n");

  while (1 < 2) {
    halt();
  }
}
