#include <paging/paging.h>

#include <klib/string.h>
#include <stdint.h>

#define MAX_MEMORY_BYTES (64u * 1024u * 1024u)
#define FRAMES_TOTAL (MAX_MEMORY_BYTES / PAGE_SIZE)
#define FRAMES_PER_DWORD 32u

static uint32_t frame_bitmap[FRAMES_TOTAL / FRAMES_PER_DWORD];
static uint32_t total_frames = 0; /* frames realmente presentes en la maquina */

static page_directory_t *kernel_dir = 0;
static page_directory_t *current_dir = 0;

/* __bss_end nos lo da kernel.ld: fin de la imagen del kernel */
extern uint32_t __bss_end;

static inline uint32_t dir_idx(uint32_t v) { return v >> 22; }
static inline uint32_t tbl_idx(uint32_t v) { return (v >> 12) & 0x3FF; }
static inline uint32_t frame_of(uint32_t addr) { return addr / PAGE_SIZE; }
static inline uint32_t addr_of(uint32_t frame) { return frame * PAGE_SIZE; }

static inline void bm_set(uint32_t f) {
  frame_bitmap[f / FRAMES_PER_DWORD] |= (1u << (f % FRAMES_PER_DWORD));
}
static inline void bm_clear(uint32_t f) {
  frame_bitmap[f / FRAMES_PER_DWORD] &= ~(1u << (f % FRAMES_PER_DWORD));
}
static inline int bm_test(uint32_t f) {
  return (frame_bitmap[f / FRAMES_PER_DWORD] >> (f % FRAMES_PER_DWORD)) & 1u;
}

uint32_t alloc_frame(void) {
  for (uint32_t i = 0; i < total_frames; i++) {
    if (!bm_test(i)) {   /* encontramos un frame libre en el bitmap */
      bm_set(i);         /* lo marcamos como ocupado */
      return addr_of(i); /* devolvemos la direccion fisica (frame * 4096) */
    }
  }
  return 0; /* no hay frames disponibles */
}

void free_frame(uint32_t frame_addr) {
  uint32_t f = frame_of(frame_addr);
  if (f < total_frames)
    bm_clear(f);
}

static page_table_t *alloc_page_table(void) {
  uint32_t frame = alloc_frame();
  if (!frame)
    return 0;
  page_table_t *t = (page_table_t *)frame;
  memset(t, 0, sizeof(page_table_t));
  return t;
}

void map_page(page_directory_t *dir, uint32_t vaddr, uint32_t paddr,
              uint32_t flags) {
  uint32_t di = dir_idx(vaddr); /* indice en el directorio: bits 31..22 */
  uint32_t ti = tbl_idx(vaddr); /* indice en la tabla: bits 21..12 */

  page_table_t *table;
  if (!(dir->entries[di] & PTE_PRESENT)) {
    table = alloc_page_table(); /* la tabla aun no existe, la creamos */
    if (!table)
      return;
    dir->entries[di] = (uint32_t)table | flags | PTE_PRESENT | PTE_RW;
  } else {
    table = (page_table_t *)(dir->entries[di] &
                             PTE_FRAME_MASK); /* tabla ya existente */
  }

  table->entries[ti] =
      (paddr & PTE_FRAME_MASK) | flags | PTE_PRESENT; /* mapeamos la pagina */
}

uint32_t get_physical(page_directory_t *dir, uint32_t vaddr) {
  uint32_t di = dir_idx(vaddr);
  uint32_t ti = tbl_idx(vaddr);
  if (!(dir->entries[di] & PTE_PRESENT)) /* el directorio no tiene la tabla */
    return 0;
  page_table_t *table = (page_table_t *)(dir->entries[di] & PTE_FRAME_MASK);
  if (!(table->entries[ti] & PTE_PRESENT)) /* la pagina no esta mapeada */
    return 0;
  return table->entries[ti] &
         PTE_FRAME_MASK; /* devolvemos la direccion fisica */
}

void switch_page_directory(page_directory_t *dir) {
  current_dir = dir;
  __asm__ __volatile__(
      "mov %0, %%cr3"
      :
      : "r"((uint32_t)dir)); /* CR3 apunta al nuevo directorio de paginas */
}

page_directory_t *get_kernel_directory(void) { return kernel_dir; }
page_directory_t *get_current_directory(void) { return current_dir; }

page_directory_t *create_page_directory(void) {
  page_directory_t *dir = (page_directory_t *)alloc_frame();
  if (!dir)
    return 0;
  memset(dir, 0, sizeof(page_directory_t));

  for (int i = 0; i < PAGE_ENTRIES; i++) {
    dir->entries[i] = kernel_dir->entries[i]; /* clonamos el mapeo del kernel en
                                                 el nuevo directorio */
  }
  return dir;
}

static void reserve_range(uint32_t addr, uint32_t size) {
  uint32_t start = addr & ~(PAGE_SIZE - 1); /* alineamos a 4K hacia abajo */
  uint32_t end = (addr + size + PAGE_SIZE - 1) &
                 ~(PAGE_SIZE - 1); /* alineamos a 4K hacia arriba */

  for (uint32_t a = start; a < end; a += PAGE_SIZE) {
    uint32_t f = frame_of(a);
    if (f < total_frames)
      bm_set(f); /* marcamos cada frame de este rango como ocupado */
  }
}

void paging_init(uint32_t mem_upper_kib, uint32_t fb_addr, uint32_t fb_size) {
  /* mem_upper es la memoria por encima de 1 MiB en KiB. Sumamos los
   * 1 MiB bajos para tener el total */
  uint32_t mem_total_kib = mem_upper_kib + 1024;
  uint32_t mem_total = mem_total_kib * 1024u;
  if (mem_total > MAX_MEMORY_BYTES)
    mem_total = MAX_MEMORY_BYTES;
  total_frames = mem_total / PAGE_SIZE;

  memset(frame_bitmap, 0, sizeof(frame_bitmap));

  uint32_t kernel_end = (uint32_t)&__bss_end;
  reserve_range(0, kernel_end);
  if (fb_size)
    reserve_range(fb_addr, fb_size);

  kernel_dir = (page_directory_t *)alloc_frame();
  memset(kernel_dir, 0, sizeof(page_directory_t));

  uint32_t ident_limit = total_frames * PAGE_SIZE;
  if (ident_limit > 32u * 1024u * 1024u)
    ident_limit =
        32u * 1024u * 1024u; /* limitamos a 32 MB para no mapear de mas */
  for (uint32_t a = 0; a < ident_limit; a += PAGE_SIZE) {
    map_page(kernel_dir, a, a, PTE_RW); /* mapeo identidad: virt = fis */
  }

  if (fb_size && fb_addr >= ident_limit) {
    uint32_t end = fb_addr + fb_size;
    for (uint32_t a = fb_addr & ~(PAGE_SIZE - 1); a < end; a += PAGE_SIZE) {
      map_page(
          kernel_dir, a, a,
          PTE_RW); /* mapeamos el framebuffer si esta fuera del rango ident */
    }
  }

  switch_page_directory(
      kernel_dir); /* cargamos el directorio de paginas en CR3 */

  uint32_t cr0;
  __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000u; /* activamos el bit PG (paging) en CR0 */
  __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}
