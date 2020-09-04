#include <isa.h>
#include <memory/paddr.h>

static const uint8_t img []  = {
  0xb8, 0x34, 0x12, 0x00, 0x00,        // 100000:  movl  $0x1234,%eax
  0xb9, 0x27, 0x00, 0x10, 0x00,        // 100005:  movl  $0x100027,%ecx
  0x89, 0x01,                          // 10000a:  movl  %eax,(%ecx)
  0x66, 0xc7, 0x41, 0x04, 0x01, 0x00,  // 10000c:  movw  $0x1,0x4(%ecx)
  0xbb, 0x02, 0x00, 0x00, 0x00,        // 100012:  movl  $0x2,%ebx
  0x66, 0xc7, 0x84, 0x99, 0x00, 0xe0,  // 100017:  movw  $0x1,-0x2000(%ecx,%ebx,4)
  0xff, 0xff, 0x01, 0x00,
  0xb8, 0x00, 0x00, 0x00, 0x00,        // 100021:  movl  $0x0,%eax
  0xd6,                                // 100026:  nemu_trap
};

static void restart() {
  /* Set the initial instruction pointer. */
  cpu.pc = PMEM_BASE + IMAGE_START;
}

void init_isa() {
  /* Test the implementation of the `CPU_state' structure. */
  void reg_test();
  reg_test();

  /* Load built-in image. */
  memcpy(guest_to_host(IMAGE_START), img, sizeof(img));

  /* Initialize this virtual computer system. */
  restart();
}
