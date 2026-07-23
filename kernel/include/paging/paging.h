#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024

/* Flags para entradas de directorio/tabla de paginas */
#define PTE_PRESENT 0x001
#define PTE_RW 0x002
#define PTE_USER 0x004
#define PTE_ACCESSED 0x020
#define PTE_DIRTY 0x040
#define PTE_4MB 0x080
#define PTE_GLOBAL 0x100

/* Mascara para quedarse con la direccion fisica de un entry */
#define PTE_FRAME_MASK 0xFFFFF000

#define TASK_PRIVATE_VADDR 0xA0000000u

typedef struct {
  uint32_t entries[PAGE_ENTRIES];
} __attribute__((aligned(4096))) page_table_t;

typedef struct {
  uint32_t entries[PAGE_ENTRIES];
} __attribute__((aligned(4096))) page_directory_t;

void paging_init(uint32_t mem_upper_kib, uint32_t fb_addr, uint32_t fb_size);
uint32_t alloc_frame(void);
void free_frame(uint32_t frame_addr);

void map_page(page_directory_t *dir, uint32_t vaddr, uint32_t paddr,
              uint32_t flags);

uint32_t get_physical(page_directory_t *dir, uint32_t vaddr);
page_directory_t *create_page_directory(void);
page_directory_t *get_kernel_directory(void);
page_directory_t *get_current_directory(void);
void switch_page_directory(page_directory_t *dir);

#endif
