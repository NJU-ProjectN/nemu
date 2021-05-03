#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include <common.h>

static inline int check_reg_index(int index) {
  assert(index >= 0 && index < 32);
  return index;
}

#define reg_d(index) (cpu.gpr[check_reg_index(index)]._64)

static inline const char* reg_name(int index) {
  extern const char* regs[];
  assert(index >= 0 && index < 32);
  return regs[index];
}

#endif
