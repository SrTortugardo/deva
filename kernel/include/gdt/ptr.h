#ifndef GDT_PTR_H
#define GDT_PTR_H

#include <stdint.h>

struct GDTPtr {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed));

#endif
