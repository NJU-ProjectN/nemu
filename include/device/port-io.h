#ifndef __PORT_IO_H__
#define __PORT_IO_H__

#include "common.h"

typedef void(*pio_callback_t)(ioaddr_t, int, bool);

void* add_pio_map(ioaddr_t, int, pio_callback_t);

#endif
