#include <klib/string.h>
#include <mm/mm.h>
#include <paging/paging.h>
#include <stdint.h>

#define HEAP_START 0x01000000
#define HEAP_GROW_PAGES 8

#define USED_MAGIC 0xDEADBEEF
#define FREE_MAGIC 0xBEEFDEAD

typedef struct block {
  uint32_t size;
  uint32_t magic;
  struct block *next;
} block_t;

#define BLOCK_SZ sizeof(block_t)

static block_t *free_list = NULL;
static uint32_t heap_cur = HEAP_START;
static uint32_t heap_max = HEAP_START;

static void heap_grow(void) {
  for (int i = 0; i < HEAP_GROW_PAGES; i++) {
    uint32_t frame = alloc_frame();
    if (!frame)
      return;
    map_page(get_kernel_directory(), heap_max, frame, PTE_PRESENT | PTE_RW);
    heap_max += 4096; /* extendemos el heap 8 paginas a la vez */
  }
  block_t *b = (block_t *)heap_cur;
  b->size = heap_max - heap_cur; /* todo el espacio nuevo es un solo bloque libre */
  b->magic = FREE_MAGIC;
  b->next = free_list;           /* lo insertamos al inicio de la free list */
  free_list = b;
  heap_cur = heap_max;
}

void *malloc(size_t size) {
  if (size == 0)
    return NULL;

  if (!free_list)
    heap_grow(); /* no hay bloques libres, pedimos mas paginas */
  if (!free_list)
    return NULL;

  size_t needed = BLOCK_SZ + size; /* espacio para el header + datos */
  needed = (needed + 3) & ~3;      /* alineamos a 4 bytes */

  block_t *prev = NULL;
  block_t *cur = free_list;

  while (cur) {
    if (cur->magic != FREE_MAGIC) { /* bloque corrupto o ya usado */
      prev = cur;
      cur = cur->next;
      continue;
    }

    if (cur->size >= needed) {
      uint32_t remaining = cur->size - needed;
      if (remaining >= BLOCK_SZ + 4) {
        /* partimos el bloque: el sobrante va a la free list */
        block_t *new_free = (block_t *)((uint8_t *)cur + needed);
        new_free->size = remaining;
        new_free->magic = FREE_MAGIC;
        new_free->next = cur->next;

        cur->size = needed; /* reducimos el actual al tamaño necesario */

        if (prev)
          prev->next = new_free;
        else
          free_list = new_free;
      } else {
        /* no sobra suficiente para otro bloque, lo asignamos entero */
        if (prev)
          prev->next = cur->next;
        else
          free_list = cur->next;
      }

      cur->magic = USED_MAGIC;
      cur->next = NULL;
      return (void *)((uint8_t *)cur + BLOCK_SZ); /* retornamos la data tras el header */
    }

    prev = cur;
    cur = cur->next;
  }

  heap_grow(); /* no encontramos bloque suficiente, expandimos el heap */
  if (heap_max > heap_cur)
    return malloc(size);
  return NULL;
}

void free(void *ptr) {
  if (!ptr)
    return;

  block_t *b = (block_t *)((uint8_t *)ptr - BLOCK_SZ); /* retrocedemos al header */
  if (b->magic != USED_MAGIC)
    return;                                              /* doble free o puntero invalido */

  b->magic = FREE_MAGIC;

  /* coalescing con el siguiente bloque fisico si esta libre */
  block_t *next_phys = (block_t *)((uint8_t *)b + b->size);
  if ((uint32_t)next_phys < heap_max && next_phys->magic == FREE_MAGIC) {
    block_t **pp = &free_list;
    while (*pp) {                /* buscamos el siguiente en la free list para quitarlo */
      if (*pp == next_phys) {
        *pp = next_phys->next;
        break;
      }
      pp = &(*pp)->next;
    }
    b->size += next_phys->size; /* fusionamos los dos bloques */
  }

  b->next = free_list; /* insertamos al inicio de la free list */
  free_list = b;
}
