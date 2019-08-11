#include "common.h"
#include "device/map.h"

#define ARGSROM_MMIO 0xa2000000

static uint8_t *argsrom_base;
char* get_mainargs(void);

void init_argsrom() {
  argsrom_base = new_space(4096);
  strcpy((char *)argsrom_base, get_mainargs());
  add_mmio_map("argsrom", ARGSROM_MMIO, argsrom_base, 4096, NULL);
}
