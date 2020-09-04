#include <isa.h>
#include <memory/paddr.h>

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
static const uint32_t img [] = {
  0x3c048000,  // lui a0, 0x8000
  0xac800000,  // sw  zero, 0(a0)
  0x8c820000,  // lw  v0,0(a0)
  0xf0000000,  // nemu_trap
};

static void restart() {
  /* Set the initial program counter. */
  cpu.pc = PMEM_BASE + IMAGE_START;

  /* The zero register is always 0. */
  cpu.gpr[0]._32 = 0;
}

void init_isa() {
  /* Load built-in image. */
  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();
}
