#ifndef ELF_H
#define ELF_H

#include <paging/paging.h>
#include <stdint.h>

uint32_t elf_load(page_directory_t *dir, uint8_t *file);

#endif
