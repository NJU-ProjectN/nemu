#ifndef __X86_H__
#define __X86_H__

#define ISA_QEMU_BIN "qemu-system-i386"
#define ISA_QEMU_ARGS

#include "../../../../src/isa/x86/difftest/difftest.h"

union isa_gdb_regs {
  struct {
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t eip, eflags;
    uint32_t cs, ss, ds, es, fs, gs;
  };
  struct {
    uint32_t array[77];
  };
};

#endif
