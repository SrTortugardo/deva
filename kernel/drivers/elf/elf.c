#include <colors.h>
#include <elf/elf.h>
#include <klib/string.h>
#include <paging/paging.h>
#include <stdint.h>
#include <term.h>

#define ELF_MAGIC0 0x7F
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define EM_386 3
#define ET_EXEC 2
#define PT_LOAD 1

typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

typedef struct {
  uint8_t e_ident[16];
  Elf32_Half e_type;
  Elf32_Half e_machine;
  Elf32_Word e_version;
  Elf32_Addr e_entry;
  Elf32_Off e_phoff;
  Elf32_Off e_shoff;
  Elf32_Word e_flags;
  Elf32_Half e_ehsize;
  Elf32_Half e_phentsize;
  Elf32_Half e_phnum;
  Elf32_Half e_shentsize;
  Elf32_Half e_shnum;
  Elf32_Half e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
  Elf32_Word p_type;
  Elf32_Off p_offset;
  Elf32_Addr p_vaddr;
  Elf32_Addr p_paddr;
  Elf32_Word p_filesz;
  Elf32_Word p_memsz;
  Elf32_Word p_flags;
  Elf32_Word p_align;
} __attribute__((packed)) Elf32_Phdr;

static int elf_check(const Elf32_Ehdr *h) {
  return h && h->e_ident[0] == ELF_MAGIC0 && h->e_ident[1] == ELF_MAGIC1 &&
         h->e_ident[2] == ELF_MAGIC2 && h->e_ident[3] == ELF_MAGIC3 &&
         h->e_ident[4] == ELFCLASS32 && h->e_ident[5] == ELFDATA2LSB &&
         h->e_machine == EM_386 &&
         h->e_type == ET_EXEC; /* verificamos magic, 32-bit, little-endian, x86,
                                  ejecutable */
}

uint32_t elf_load(page_directory_t *dir, uint8_t *file) {
  Elf32_Ehdr *eh = (Elf32_Ehdr *)file;

  if (!elf_check(eh)) {
    term_write("elf no pasa checks\n", COLOR_RED);
    return 0;
  }

  Elf32_Phdr *ph =
      (Elf32_Phdr *)(file + eh->e_phoff); /* tabla de program headers tras el
                                             header ELF */

  for (int i = 0; i < eh->e_phnum; i++) {
    if (ph[i].p_type != PT_LOAD)
      continue; /* solo nos interesan los segmentos cargables */

    uint32_t vaddr = ph[i].p_vaddr;
    uint32_t filesz = ph[i].p_filesz;
    uint32_t memsz = ph[i].p_memsz;
    uint8_t *src =
        file + ph[i].p_offset; /* datos del segmento dentro del binario */

    uint32_t start = vaddr & ~0xFFFu; /* alineamos a 4K hacia abajo */
    uint32_t end =
        (vaddr + memsz + 0xFFFu) & ~0xFFFu; /* alineamos a 4K hacia arriba */

    for (uint32_t page_v = start; page_v < end; page_v += PAGE_SIZE) {
      uint32_t frame = get_physical(dir, page_v);
      if (!frame) {
        frame = alloc_frame(); /* pedimos un frame fisico nuevo */
        if (!frame) {
          term_write("[elf] sin memoria\n", COLOR_RED);
          return 0;
        }
        uint8_t *frame_ptr = (uint8_t *)frame;
        memset(frame_ptr, 0, PAGE_SIZE); /* lo ponemos a cero (bss) */
        map_page(dir, page_v, frame,
                 PTE_RW | PTE_USER); /* mapeamos en el dir del proceso */
      }
      uint8_t *frame_ptr = (uint8_t *)frame;

      /* donde empieza y termina el segmento dentro de esta pagina */
      uint32_t seg_lo = (page_v > vaddr) ? (page_v - vaddr) : 0;
      uint32_t seg_hi = (page_v + PAGE_SIZE);
      seg_hi = (seg_hi > vaddr + memsz) ? (vaddr + memsz) : seg_hi;
      seg_hi -= vaddr;

      /* copiamos del archivo solo lo que pertenece a filesz, el resto ya es
       * cero */
      if (seg_lo < filesz) {
        uint32_t copy_hi = (seg_hi < filesz) ? seg_hi : filesz;
        uint32_t n = copy_hi - seg_lo;
        uint32_t dst_off = (page_v > vaddr) ? 0 : (vaddr - page_v);
        memcpy(frame_ptr + dst_off, src + seg_lo, n);
      }
    }
  }

  return (uint32_t)eh->e_entry;
}
