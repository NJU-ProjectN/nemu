#ifndef __RTL_BASIC_H__
#define __RTL_BASIC_H__

#include "c_op.h"
#include <memory/vaddr.h>

/* RTL basic instructions */

#define def_rtl_compute_reg(name) \
  static inline def_rtl(name, rtlreg_t* dest, const rtlreg_t* src1, const rtlreg_t* src2) { \
    *dest = concat(c_, name) (*src1, *src2); \
  }

#define def_rtl_compute_imm(name) \
  static inline def_rtl(name ## i, rtlreg_t* dest, const rtlreg_t* src1, const sword_t imm) { \
    *dest = concat(c_, name) (*src1, imm); \
  }

#define def_rtl_compute_reg_imm(name) \
  def_rtl_compute_reg(name) \
  def_rtl_compute_imm(name) \

// compute

def_rtl_compute_reg_imm(add)
def_rtl_compute_reg_imm(sub)
def_rtl_compute_reg_imm(and)
def_rtl_compute_reg_imm(or)
def_rtl_compute_reg_imm(xor)
def_rtl_compute_reg_imm(sll)
def_rtl_compute_reg_imm(srl)
def_rtl_compute_reg_imm(sra)

#ifdef CONFIG_ISA64
def_rtl_compute_reg_imm(addw)
def_rtl_compute_reg_imm(subw)
def_rtl_compute_reg_imm(sllw)
def_rtl_compute_reg_imm(srlw)
def_rtl_compute_reg_imm(sraw)
#define rtl_addiw rtl_addwi
#define rtl_slliw rtl_sllwi
#define rtl_srliw rtl_srlwi
#define rtl_sraiw rtl_srawi
#endif

static inline def_rtl(setrelop, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2) {
  *dest = interpret_relop(relop, *src1, *src2);
}

static inline def_rtl(setrelopi, uint32_t relop, rtlreg_t *dest,
    const rtlreg_t *src1, sword_t imm) {
  *dest = interpret_relop(relop, *src1, imm);
}

// mul/div

def_rtl_compute_reg(mulu_lo)
def_rtl_compute_reg(mulu_hi)
def_rtl_compute_reg(muls_hi)
def_rtl_compute_reg(divu_q)
def_rtl_compute_reg(divu_r)
def_rtl_compute_reg(divs_q)
def_rtl_compute_reg(divs_r)

#ifdef CONFIG_ISA64
def_rtl_compute_reg(mulw)
def_rtl_compute_reg(divw)
def_rtl_compute_reg(divuw)
def_rtl_compute_reg(remw)
def_rtl_compute_reg(remuw)
#endif

static inline def_rtl(div64u_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  uint32_t divisor = (*src2);
  *dest = dividend / divisor;
}

static inline def_rtl(div64u_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  uint64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  uint32_t divisor = (*src2);
  *dest = dividend % divisor;
}

static inline def_rtl(div64s_q, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  int64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  int32_t divisor = (*src2);
  *dest = dividend / divisor;
}

static inline def_rtl(div64s_r, rtlreg_t* dest,
    const rtlreg_t* src1_hi, const rtlreg_t* src1_lo, const rtlreg_t* src2) {
  int64_t dividend = ((uint64_t)(*src1_hi) << 32) | (*src1_lo);
  int32_t divisor = (*src2);
  *dest = dividend % divisor;
}

// memory

static inline def_rtl(lm, rtlreg_t *dest, const rtlreg_t* addr, word_t offset, int len) {
  *dest = vaddr_read(*addr + offset, len);
}

static inline def_rtl(sm, const rtlreg_t *src1, const rtlreg_t* addr, word_t offset, int len) {
  vaddr_write(*addr + offset, len, *src1);
}

static inline def_rtl(lms, rtlreg_t *dest, const rtlreg_t* addr, word_t offset, int len) {
  word_t val = vaddr_read(*addr + offset, len);
  switch (len) {
    case 4: *dest = (sword_t)(int32_t)val; return;
    case 1: *dest = (sword_t)( int8_t)val; return;
    case 2: *dest = (sword_t)(int16_t)val; return;
    IFDEF(CONFIG_ISA64, case 8: *dest = (sword_t)(int64_t)val; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

static inline def_rtl(host_lm, rtlreg_t* dest, const void *addr, int len) {
  switch (len) {
    case 4: *dest = *(uint32_t *)addr; return;
    case 1: *dest = *( uint8_t *)addr; return;
    case 2: *dest = *(uint16_t *)addr; return;
    IFDEF(CONFIG_ISA64, case 8: *dest = *(uint64_t *)addr; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

static inline def_rtl(host_sm, void *addr, const rtlreg_t *src1, int len) {
  switch (len) {
    case 4: *(uint32_t *)addr = *src1; return;
    case 1: *( uint8_t *)addr = *src1; return;
    case 2: *(uint16_t *)addr = *src1; return;
    IFDEF(CONFIG_ISA64, case 8: *(uint64_t *)addr = *src1; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

// control

static inline def_rtl(j, vaddr_t target) {
  s->dnpc = target;
}

static inline def_rtl(jr, rtlreg_t *target) {
  s->dnpc = *target;
}

static inline def_rtl(jrelop, uint32_t relop,
    const rtlreg_t *src1, const rtlreg_t *src2, vaddr_t target) {
  bool is_jmp = interpret_relop(relop, *src1, *src2);
  rtl_j(s, (is_jmp ? target : s->snpc));
}
#endif
