#ifndef __DEVICE_MMIO_H__
#define __DEVICE_MMIO_H__

#include <common.h>

word_t mmio_read(paddr_t addr, int len);
void mmio_write(paddr_t addr, int len, word_t data);

#endif
