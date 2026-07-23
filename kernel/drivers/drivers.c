#include <colors.h>
#include <drivers.h>
#include <stddef.h>
#include <stdint.h>
#include <term.h>

#define MAX_DRIVERS 32 /* Razonable */

static struct driver *drivers[MAX_DRIVERS];
static size_t driver_count = 0;

void driver_register(struct driver *drv) { drivers[driver_count++] = drv; }

void drivers_init(void) {
  term_write("Inicializando drivers...\n", COLOR_TEXT);

  for (size_t i = 0; i < driver_count; i++) {
    struct driver *drv = drivers[i];

    term_write("[ .. ] ", COLOR_YELLOW);
    term_write(drv->name, COLOR_TEXT);
    term_write("\n", COLOR_TEXT);

    int ret = drv->init();

    if (ret == 0) {
      term_write("[ OK ] ", COLOR_GREEN);
      term_write(drv->name, COLOR_TEXT);
      term_write("\n", COLOR_TEXT);
    } else {
      term_write("[FAIL] ", COLOR_RED);
      term_write(drv->name, COLOR_TEXT);
      term_write("\n", COLOR_TEXT);
    }
  }
}
