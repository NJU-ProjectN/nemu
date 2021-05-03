#include <isa.h>
#include <memory/paddr.h>

// this is not consistent with uint8_t
// but it is ok since we do not access the array directly
static const uint32_t img [] = {
  0x800002b7,  // lui t0,0x80000
  0x0002b023,  // sd  zero,0(t0)
  0x0002b503,  // ld  a0,0(t0)
  0x0000006b,  // nemu_trap
};

static void restart() {
  /* Set the initial program counter. */
  cpu.pc = PMEM_BASE + IMAGE_START;

  /* The zero register is always 0. */
  cpu.gpr[0]._64 = 0;
}

void init_isa() {
  /* Load built-in image. */
  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();
}
