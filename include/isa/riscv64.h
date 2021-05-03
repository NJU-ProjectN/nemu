#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>

// memory
#define riscv64_IMAGE_START 0x100000
#define riscv64_PMEM_BASE 0x80000000

// reg

typedef struct {
  union {
    rtlreg_t _64;
  } gpr[32];

  vaddr_t pc;
} riscv64_CPU_state;

// decode
typedef struct {
  union {
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      int32_t  simm11_0  :12;
    } i;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t imm4_0    : 5;
      uint32_t funct3    : 3;
      uint32_t rs1       : 5;
      uint32_t rs2       : 5;
      int32_t  simm11_5  : 7;
    } s;
    struct {
      uint32_t opcode1_0 : 2;
      uint32_t opcode6_2 : 5;
      uint32_t rd        : 5;
      int32_t  simm31_12 :20;
    } u;
    uint32_t val;
  } instr;
} riscv64_ISADecodeInfo;

#define isa_vaddr_check(vaddr, type, len) (MEM_RET_OK)
#define riscv64_has_mem_exception() (cpu.mem_exception != 0)

#endif
