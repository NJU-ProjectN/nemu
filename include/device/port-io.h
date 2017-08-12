#ifndef __PORT_IO_H__
#define __PORT_IO_H__

#include "common.h"

typedef void(*pio_callback_t)(ioaddr_t, int, bool);

void* add_pio_map(ioaddr_t, int, pio_callback_t);

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

#endif
