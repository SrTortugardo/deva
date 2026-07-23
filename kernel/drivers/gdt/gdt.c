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
  gdt[GDT_KERNEL_CS].limit_low = 0xFFFF;
  gdt[GDT_KERNEL_CS].base_low = 0x0000;
  gdt[GDT_KERNEL_CS].base_middle = 0x00;
  gdt[GDT_KERNEL_CS].access =
      0x9A; /* presente, ring 0, codigo, ejecutable/legible */
  gdt[GDT_KERNEL_CS].granularity = 0xCF; /* 4KB pages, 32-bit, limite 0xFFFFF */
  gdt[GDT_KERNEL_CS].base_high = 0x00;

  /* kernel data segment tambien en ring 0 */
  gdt[GDT_KERNEL_DS].limit_low = 0xFFFF;
  gdt[GDT_KERNEL_DS].base_low = 0x0000;
  gdt[GDT_KERNEL_DS].base_middle = 0x00;
  gdt[GDT_KERNEL_DS].access =
      0x92; /* presente, ring 0, datos, lectura/escritura */
  gdt[GDT_KERNEL_DS].granularity = 0xCF;
  gdt[GDT_KERNEL_DS].base_high = 0x00;

  /* RING 3: segmento de codigo para modo usuario */
  gdt[GDT_USER_CS].limit_low = 0xFFFF;
  gdt[GDT_USER_CS].base_low = 0x0000;
  gdt[GDT_USER_CS].base_middle = 0x00;
  gdt[GDT_USER_CS].access =
      0xFA; /* presente, ring 3, codigo, ejecutable/legible */
  gdt[GDT_USER_CS].granularity = 0xCF;
  gdt[GDT_USER_CS].base_high = 0x00;

  /* write-read del user */
  gdt[GDT_USER_DS].limit_low = 0xFFFF;
  gdt[GDT_USER_DS].base_low = 0x0000;
  gdt[GDT_USER_DS].base_middle = 0x00;
  gdt[GDT_USER_DS].access =
      0xF2; /* presente, ring 3, datos, lectura/escritura */
  gdt[GDT_USER_DS].granularity = 0xCF;
  gdt[GDT_USER_DS].base_high = 0x00;

  gdt_load(&gdt_ptr);
}

void gdt_load(struct GDTPtr *ptr) {
  __asm__ __volatile__("lgdt %0"
                       :
                       : "m"(*ptr)); /* carga la tabla GDT en el procesador */

  /* recargamos los registros de segmento con los nuevos selectores */
  __asm__ __volatile__(
      "movw $0x10, %%ax\n\t" /* 0x10 = selector de datos del kernel */
      "movw %%ax, %%ds\n\t"
      "movw %%ax, %%es\n\t"
      "movw %%ax, %%fs\n\t"
      "movw %%ax, %%gs\n\t"
      "movw %%ax, %%ss\n\t"
      "ljmp $0x08, $1f\n\t" /* far jump: recarga CS con 0x08 */
      "1:"
      :
      :
      : "eax", "memory");
}
