#ifndef __MMIO_H__
#define __MMIO_H__

#include "common.h"

typedef void(*mmio_callback_t)(paddr_t, int, bool);

void* add_mmio_map(paddr_t, int, mmio_callback_t);
int is_mmio(paddr_t);

uint32_t mmio_read(paddr_t, int, int);
void mmio_write(paddr_t, int, uint32_t, int);

#endif
