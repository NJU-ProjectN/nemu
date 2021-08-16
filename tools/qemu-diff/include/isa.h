#ifndef __ISA_H__
#define __ISA_H__

#if defined(CONFIG_ISA_mips32)
#define ISA_QEMU_BIN "qemu-system-mipsel"
#define ISA_QEMU_ARGS "-machine", "mipssim",\
  "-kernel", NEMU_HOME "/resource/mips-elf/mips.dummy",
#elif defined(CONFIG_ISA_riscv32)
#define ISA_QEMU_BIN "qemu-system-riscv32"
#define ISA_QEMU_ARGS "-bios", "none",
#elif defined(CONFIG_ISA_riscv64)
#define ISA_QEMU_BIN "qemu-system-riscv64"
#define ISA_QEMU_ARGS 
#elif defined(CONFIG_ISA_x86)
#define ISA_QEMU_BIN "qemu-system-i386"
#define ISA_QEMU_ARGS
#else
#error Unsupport ISA
#endif

union isa_gdb_regs {
  struct {
#if defined(CONFIG_ISA_mips32)
    uint32_t gpr[32];
    uint32_t status, lo, hi, badvaddr, cause, pc;
#elif defined(CONFIG_ISA_riscv32)
    uint32_t gpr[32];
    uint32_t pc;
#elif defined(CONFIG_ISA_riscv64)
    uint64_t gpr[32];
    uint64_t fpr[32];
    uint64_t pc;
#elif defined(CONFIG_ISA_x86)
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t eip, eflags;
    uint32_t cs, ss, ds, es, fs, gs;
#endif
  };
  struct {
    uint32_t array[77];
  };
};

#endif
