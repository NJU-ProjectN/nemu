#include "common.h"

bool gdb_memcpy_to_qemu(uint32_t, void *, int);
bool gdb_getregs(union isa_gdb_regs *);
bool gdb_setregs(union isa_gdb_regs *);
void difftest_exec(uint64_t n);

static uint32_t initcode[] = {
  0xfff00513,  // 00:  li	a0,-1
  0x01f00593,  // 04:  li	a1,31
  0x30251073,  // 08:  csrw	medeleg,a0

  0x00000697,  // 0c:  auipc	a3,0x0
  0x01468693,  // 10:  addi	a3,a3,20 # 80000020 <here>
  0x30569073,  // 14:  csrw	mtvec,a3
  0x3b051073,  // 18:  csrw	pmpaddr0,a0
  0x3a059073,  // 1c:  csrw	pmpcfg0,a1

// here:
  0x000c1637,  // 20:  lui	a2,0xc1
  0x8006061b,  // 24:  addiw	a2,a2,-2048
  0x30061073,  // 28:  csrw	mstatus,a2

  0x00000697,  // 2c:  auipc	a3,0x0
  0x01068693,  // 30:  addi	a3,a3,16 # 8000003c <spin>
  0x34169073,  // 34:  csrw	mepc,a3

  0x30200073,  // 38:  mret

// spin:
  0x0000006f,  // 3c:  j	8000003c <spin>
  0x0000006f,  // # spin
  0x0000006f,  // # spin
  0x0000006f,  // # spin
};


void init_isa() {
  // put initcode to QEMU to setup a PMP to permit access to all of memory in S mode
  bool ok = gdb_memcpy_to_qemu(0x80000000, initcode, sizeof(initcode));
  assert(ok == 1);

  union isa_gdb_regs r;
  gdb_getregs(&r);

  r.pc = 0x80000000;
  ok = gdb_setregs(&r);
  assert(ok == 1);

  // execute enough instructions
  difftest_exec(20);
}
