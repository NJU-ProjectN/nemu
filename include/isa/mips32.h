#ifndef __ISA_MIPS32_H__
#define __ISA_MIPS32_H__

#include <common.h>

// memory
#define mips32_IMAGE_START 0x100000
#define mips32_PMEM_BASE 0x80000000

// reg
typedef struct {
  struct {
    rtlreg_t _32;
  } gpr[32];

  rtlreg_t pad[5];

  vaddr_t pc;
} mips32_CPU_state;

// decode
typedef struct {
  union {
    struct {
      int32_t  simm   : 16;
      uint32_t rt     :  5;
      uint32_t rs     :  5;
      uint32_t opcode :  6;
    } i;
    struct {
      uint32_t imm    : 16;
      uint32_t rt     :  5;
      uint32_t rs     :  5;
      uint32_t opcode :  6;
    } iu;
    struct {
      uint32_t func   : 6;
      uint32_t sa     : 5;
      uint32_t rd     : 5;
      uint32_t rt     : 5;
      uint32_t rs     : 5;
      uint32_t opcode : 6;
    } r;
    uint32_t val;
  } instr;
} mips32_ISADecodeInfo;

#define isa_vaddr_check(vaddr, type, len) (MEM_RET_OK)
#define mips32_has_mem_exception() (false)

#endif
