#include "cpu/exec.h"

CPU_state cpu;

rtlreg_t s0, s1, t0, t1, at;

/* shared by all helper functions */
DecodeInfo decinfo;

void decinfo_set_jmp(bool is_jmp) {
  decinfo.is_jmp = is_jmp;
}

void isa_exec(vaddr_t *pc);

void exec_wrapper(bool print_flag) {
  vaddr_t ori_pc = cpu.pc;

#ifdef DEBUG
  decinfo.p = decinfo.asm_buf;
  decinfo.p += sprintf(decinfo.p, "%8x:   ", ori_pc);
#endif

  decinfo.seq_pc = ori_pc;
  isa_exec(&decinfo.seq_pc);

#ifdef DEBUG
  int instr_len = decinfo.seq_pc - ori_pc;
  sprintf(decinfo.p, "%*.s", 50 - (12 + 3 * instr_len), "");
  strncat(decinfo.asm_buf, decinfo.assembly, 80);
  Log_write("%s\n", decinfo.asm_buf);
  if (print_flag) {
    puts(decinfo.asm_buf);
  }
#endif

  update_pc();

#if defined(DIFF_TEST)
  void difftest_step(vaddr_t ori_pc, vaddr_t next_pc);
  difftest_step(ori_pc, cpu.pc);
#endif
}
