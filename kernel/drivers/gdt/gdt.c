#include <gdt/gdt.h>
#include <stddef.h>

struct GDTEntry gdt[GDT_ENTRIES];
struct GDTPtr gdt_ptr;

void init_gdt() {
  gdt_ptr.limit = (sizeof(gdt) - 1);
  gdt_ptr.base = (uint32_t)&gdt;

  /* segmento nulo */
  gdt[GDT_NULL].limit_low = 0;
  gdt[GDT_NULL].base_low = 0;
  gdt[GDT_NULL].base_middle = 0;
  gdt[GDT_NULL].access = 0;
  gdt[GDT_NULL].granularity = 0;
  gdt[GDT_NULL].base_high = 0;

  /* kernel code segment, alias ring 0 */
  /* 0x9a es ejecutable y leible con privilegio 0*/
  /* 0xcf es de 4kb + 32 bits */
  gdt[GDT_KERNEL_CS].limit_low = 0xFFFF;
  gdt[GDT_KERNEL_CS].base_low = 0x0000;
  gdt[GDT_KERNEL_CS].base_middle = 0x00;
  gdt[GDT_KERNEL_CS].access = 0x9A;
  gdt[GDT_KERNEL_CS].granularity = 0xCF;
  gdt[GDT_KERNEL_CS].base_high = 0x00;

  /* kernel data segment tambien en ring 0 */
  /* 0x92 es write-read con privilegio 0 */
  gdt[GDT_KERNEL_DS].limit_low = 0xFFFF;
  gdt[GDT_KERNEL_DS].base_low = 0x0000;
  gdt[GDT_KERNEL_DS].base_middle = 0x00;
  gdt[GDT_KERNEL_DS].access = 0x92;
  gdt[GDT_KERNEL_DS].granularity = 0xCF;
  gdt[GDT_KERNEL_DS].base_high = 0x00;

  /* RING 3! */
  gdt[GDT_USER_CS].limit_low = 0xFFFF;
  gdt[GDT_USER_CS].base_low = 0x0000;
  gdt[GDT_USER_CS].base_middle = 0x00;
  gdt[GDT_USER_CS].access = 0xFA;
  gdt[GDT_USER_CS].granularity = 0xCF;
  gdt[GDT_USER_CS].base_high = 0x00;

  /* write-read del user */
  gdt[GDT_USER_DS].limit_low = 0xFFFF;
  gdt[GDT_USER_DS].base_low = 0x0000;
  gdt[GDT_USER_DS].base_middle = 0x00;
  gdt[GDT_USER_DS].access = 0xF2;
  gdt[GDT_USER_DS].granularity = 0xCF;
  gdt[GDT_USER_DS].base_high = 0x00;

  gdt_load(&gdt_ptr);
}

void gdt_load(struct GDTPtr *ptr) {
  __asm__ __volatile__("lgdt %0"
                       :
                       : "m"(*ptr)); /* ze carga la gdt usando lgdt*/

  /* Recargar los registros de segmento para que usen la nueva GDT.
   * Sin esto el CPU seguiria usando los descriptores cacheados de GRUB. */
  __asm__ __volatile__("movw $0x10, %%ax\n\t"
                       "movw %%ax, %%ds\n\t"
                       "movw %%ax, %%es\n\t"
                       "movw %%ax, %%fs\n\t"
                       "movw %%ax, %%gs\n\t"
                       "movw %%ax, %%ss\n\t"
                       "ljmp $0x08, $1f\n\t"
                       "1:"
                       :
                       :
                       : "eax", "memory");
}
