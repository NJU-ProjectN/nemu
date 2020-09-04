#include <device/map.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define SERIAL_PORT 0x3F8
#define SERIAL_MMIO 0xa10003F8

#define CH_OFFSET 0

static uint8_t *serial_base = NULL;


static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {
    /* We bind the serial port with the host stderr in NEMU. */
    case CH_OFFSET:
      if (is_write) putc(serial_base[0], stderr);
      else panic("do not support read");
      break;
    default: panic("do not support offset = %d", offset);
  }
}


void init_serial() {
  serial_base = new_space(8);
  add_pio_map("serial", SERIAL_PORT, serial_base, 8, serial_io_handler);
  add_mmio_map("serial", SERIAL_MMIO, serial_base, 8, serial_io_handler);

}
