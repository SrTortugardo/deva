#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>

struct driver {
  const char *name;
  const char *author;

  uint32_t version;

  int (*init)(void);
};

void driver_register(struct driver *drv);
void drivers_init();

#endif
