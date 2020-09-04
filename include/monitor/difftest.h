#ifndef __MONITOR_DIFFTEST_H__
#define __MONITOR_DIFFTEST_H__

#include <common.h>

#ifdef DIFF_TEST
void difftest_skip_ref();
void difftest_skip_dut(int nr_ref, int nr_dut);
void difftest_step(vaddr_t this_pc, vaddr_t next_pc);
#else
#define difftest_skip_ref()
#define difftest_skip_dut(nr_ref, nr_dut)
static inline void difftest_step(vaddr_t this_pc, vaddr_t next_pc) {}
#endif

extern void (*ref_difftest_memcpy_from_dut)(paddr_t dest, void *src, size_t n);
extern void (*ref_difftest_getregs)(void *c);
extern void (*ref_difftest_setregs)(const void *c);
extern void (*ref_difftest_exec)(uint64_t n);

static inline bool difftest_check_reg(const char *name, vaddr_t pc, rtlreg_t ref, rtlreg_t dut) {
  if (ref != dut) {
    Log("%s is different after executing instruction at pc = " FMT_WORD ", right = " FMT_WORD ", wrong = " FMT_WORD,
        name, pc, ref, dut);
    return false;
  }
  return true;
}

#endif
