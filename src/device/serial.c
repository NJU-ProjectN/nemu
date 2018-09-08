#include "common.h"
#include "device/port-io.h"

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */

#define SERIAL_PORT 0x3F8
#define CH_OFFSET 0
#define LSR_OFFSET 5		/* line status register */

static uint8_t *serial_ch_base;
static uint8_t *serial_lsr_base;

static void serial_ch_io_handler(ioaddr_t addr, int len, bool is_write) {
  assert(is_write);
  assert(len == 1);
  char c = serial_ch_base[0];
  /* We bind the serial port with the host stdout in NEMU. */
  putc(c, stdout);
  if (c == '\n') {
    fflush(stdout);
  }
}

void init_serial() {
  serial_ch_base = add_pio_map(SERIAL_PORT + CH_OFFSET, 1, serial_ch_io_handler);
  serial_lsr_base = add_pio_map(SERIAL_PORT + LSR_OFFSET, 1, NULL);
  serial_lsr_base[0] = 0x20; /* the status is always free */
}
