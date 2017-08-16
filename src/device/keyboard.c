#include "device/port-io.h"
#include "monitor/monitor.h"
#include <SDL2/SDL.h>

#define I8042_DATA_PORT 0x60
#define I8042_STATUS_PORT 0x64
#define I8042_STATUS_HASKEY_MASK 0x1
#define KEYBOARD_IRQ 1

static uint32_t *i8042_data_port_base;
static uint8_t *i8042_status_port_base;

#define _KEYS(_) \
  _(ESCAPE) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) _(F8) _(F9) _(F10) _(F11) _(F12) \
_(GRAVE) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(0) _(MINUS) _(EQUALS) _(BACKSPACE) \
_(TAB) _(Q) _(W) _(E) _(R) _(T) _(Y) _(U) _(I) _(O) _(P) _(LEFTBRACKET) _(RIGHTBRACKET) _(BACKSLASH) \
_(CAPSLOCK) _(A) _(S) _(D) _(F) _(G) _(H) _(J) _(K) _(L) _(SEMICOLON) _(APOSTROPHE) _(RETURN) \
_(LSHIFT) _(Z) _(X) _(C) _(V) _(B) _(N) _(M) _(COMMA) _(PERIOD) _(SLASH) _(RSHIFT) \
_(LCTRL) _(APPLICATION) _(LALT) _(SPACE) _(RALT) _(RCTRL) \
_(UP) _(DOWN) _(LEFT) _(RIGHT) _(INSERT) _(DELETE) _(HOME) _(END) _(PAGEUP) _(PAGEDOWN)

#define _KEY_NAME(k) _KEY_##k,

enum {
  _KEY_NONE = 0,
  _KEYS(_KEY_NAME)
};

#define XX(k) [concat(SDL_SCANCODE_, k)] = concat(_KEY_, k),
static uint32_t keymap[256] = {
  _KEYS(XX)
};

#define KEY_QUEUE_LEN 1024
static int key_queue[KEY_QUEUE_LEN];
static int key_f = 0, key_r = 0;

#define KEYDOWN_MASK 0x8000

void send_key(uint8_t scancode, bool is_keydown) {
  if (nemu_state == NEMU_RUNNING &&
      keymap[scancode] != _KEY_NONE) {
    uint32_t am_scancode = keymap[scancode] | (is_keydown ? KEYDOWN_MASK : 0);
    key_queue[key_r] = am_scancode;
    key_r = (key_r + 1) % KEY_QUEUE_LEN;
  }
}

void i8042_io_handler(ioaddr_t addr, int len, bool is_write) {
  if (!is_write) {
    if (addr == I8042_DATA_PORT) {
      i8042_status_port_base[0] &= ~I8042_STATUS_HASKEY_MASK;
    }
    else if (addr == I8042_STATUS_PORT) {
      if ((i8042_status_port_base[0] & I8042_STATUS_HASKEY_MASK) == 0) {
        if (key_f != key_r) {
          i8042_data_port_base[0] = key_queue[key_f];
          i8042_status_port_base[0] |= I8042_STATUS_HASKEY_MASK;
          key_f = (key_f + 1) % KEY_QUEUE_LEN;
        }
      }
    }
  }
}

void init_i8042() {
  i8042_data_port_base = add_pio_map(I8042_DATA_PORT, 4, i8042_io_handler);
  i8042_status_port_base = add_pio_map(I8042_STATUS_PORT, 1, i8042_io_handler);
  i8042_status_port_base[0] = 0x0;
}
