#include "common.h"
#include "device/port-io.h"

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */

#define SERIAL_PORT 0x3F8
#define CH_OFFSET 0
#define LSR_OFFSET 5		/* line status register */

static uint8_t *serial_port_base;

void serial_io_handler(ioaddr_t addr, int len, bool is_write) {
  if (is_write) {
    assert(len == 1);
    if (addr == SERIAL_PORT + CH_OFFSET) {
      char c = serial_port_base[CH_OFFSET];
      /* We bind the serial port with the host stdout in NEMU. */
      putc(c, stdout);
      if (c == '\n') {
        fflush(stdout);
      }
    }
  }
}

void init_serial() {
  serial_port_base = add_pio_map(SERIAL_PORT, 8, serial_io_handler);
  serial_port_base[LSR_OFFSET] = 0x20; /* the status is always free */
}
