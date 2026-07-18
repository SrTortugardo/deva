#ifndef GDT_H
#define GDT_H

#include "entry.h"
#include "ptr.h"
#include <stdint.h>

#define GDT_NULL 0
#define GDT_KERNEL_CS 1
#define GDT_KERNEL_DS 2
#define GDT_USER_CS 3
#define GDT_USER_DS 4
#define GDT_ENTRIES 5

#define KERNEL_CS (GDT_KERNEL_CS << 3)
#define KERNEL_DS (GDT_KERNEL_DS << 3)
#define USER_CS ((GDT_USER_CS << 3) | 0x3)
#define USER_DS ((GDT_USER_DS << 3) | 0x3)

extern struct GDTEntry gdt[GDT_ENTRIES];
extern struct GDTPtr gdt_ptr;

void init_gdt();
void gdt_load(struct GDTPtr *ptr);

#endif
