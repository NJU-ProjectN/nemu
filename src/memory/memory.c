#include "nemu.h"
#include "device/map.h"

uint8_t pmem[PMEM_SIZE] PG_ALIGN;

static IOMap pmem_map = {
  .name = "pmem",
  .space = pmem,
  .callback = NULL
};

void register_pmem(paddr_t base) {
  pmem_map.low = base;
  pmem_map.high = base + PMEM_SIZE - 1;

  Log("Add '%s' at [0x%08x, 0x%08x]", pmem_map.name, pmem_map.low, pmem_map.high);
}

IOMap* fetch_mmio_map(paddr_t addr);

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  IOMap* map = fetch_mmio_map(addr);
  if (map == NULL) { map = &pmem_map; }

  return map_read(addr, len, map);
}

void paddr_write(paddr_t addr, uint32_t data, int len) {
  IOMap* map = fetch_mmio_map(addr);
  if (map == NULL) { map = &pmem_map; }

  map_write(addr, data, len, map);
}
