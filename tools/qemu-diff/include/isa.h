/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __ISA_H__
#define __ISA_H__

#if defined(CONFIG_ISA_mips32)
#define ISA_QEMU_BIN "qemu-system-mipsel"
#define ISA_QEMU_ARGS "-machine", "mipssim",\
  "-kernel", NEMU_HOME "/resource/mips-elf/mips.dummy",
#elif defined(CONFIG_ISA_riscv) && !defined(CONFIG_RV64)
#define ISA_QEMU_BIN "qemu-system-riscv32"
#define ISA_QEMU_ARGS "-bios", "none",
#elif defined(CONFIG_ISA_riscv) && defined(CONFIG_RV64)
#define ISA_QEMU_BIN "qemu-system-riscv64"
#define ISA_QEMU_ARGS 
#elif defined(CONFIG_ISA_x86)
#define ISA_QEMU_BIN "qemu-system-i386"
#define ISA_QEMU_ARGS
#elif defined(CONFIG_ISA_loongarch32r)
#define ISA_QEMU_BIN "qemu-system-loongarch32"
#define ISA_QEMU_ARGS "-M","ls3a5k32",
#else
#error Unsupport ISA
#endif

union isa_gdb_regs {
  struct {
#if defined(CONFIG_ISA_mips32)
    uint32_t gpr[32];
    uint32_t status, lo, hi, badvaddr, cause, pc;
#elif defined(CONFIG_ISA_riscv) && !defined(CONFIG_RV64)
    uint32_t gpr[32];
    uint32_t pc;
#elif defined(CONFIG_ISA_riscv) &&  defined(CONFIG_RV64)
    uint64_t gpr[32];
    uint64_t fpr[32];
    uint64_t pc;
#elif defined(CONFIG_ISA_x86)
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t eip, eflags;
    uint32_t cs, ss, ds, es, fs, gs;
#elif defined(CONFIG_ISA_loongarch32r)
  uint32_t gpr[32];
  uint32_t pc;
#endif
  };
  struct {
    uint32_t array[77];
  };
};

#endif
