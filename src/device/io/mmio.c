#include "common.h"
#include "device/mmio.h"

#define MMIO_SPACE_MAX (512 * 1024)
#define NR_MAP 8

static uint8_t mmio_space_pool[MMIO_SPACE_MAX];
static uint32_t mmio_space_free_index = 0;

typedef struct {
  paddr_t low;
  paddr_t high;
  uint8_t *mmio_space;
  mmio_callback_t callback;
} MMIO_t;

static MMIO_t maps[NR_MAP];
static int nr_map = 0;

/* device interface */
void* add_mmio_map(paddr_t addr, int len, mmio_callback_t callback) {
  assert(nr_map < NR_MAP);
  assert(mmio_space_free_index + len <= MMIO_SPACE_MAX);

  uint8_t *space_base = &mmio_space_pool[mmio_space_free_index];
  maps[nr_map].low = addr;
  maps[nr_map].high = addr + len - 1;
  maps[nr_map].mmio_space = space_base;
  maps[nr_map].callback = callback;
  nr_map ++;
  mmio_space_free_index += len;
  return space_base;
}

/* bus interface */
int is_mmio(paddr_t addr) {
  int i;
  for (i = 0; i < nr_map; i ++) {
    if (addr >= maps[i].low && addr <= maps[i].high) {
      return i;
    }
  }
  return -1;
}

uint32_t mmio_read(paddr_t addr, int len, int map_NO) {
  assert(len >= 1 && len <= 4);
  MMIO_t *map = &maps[map_NO];
  uint32_t data = *(uint32_t *)(map->mmio_space + (addr - map->low)) 
    & (~0u >> ((4 - len) << 3));
  map->callback(addr, len, false);
  return data;
}

void mmio_write(paddr_t addr, int len, uint32_t data, int map_NO) {
  assert(len >= 1 && len <= 4);
  MMIO_t *map = &maps[map_NO];

  uint8_t *p = map->mmio_space + (addr - map->low);
  uint8_t *p_data = (uint8_t *)&data;

  switch (len) {
    case 4: p[3] = p_data[3];
    case 3: p[2] = p_data[2];
    case 2: p[1] = p_data[1];
    case 1: p[0] = p_data[0]; break;
  }

  maps[map_NO].callback(addr, len, true);
}
