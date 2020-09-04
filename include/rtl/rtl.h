#ifndef __RTL_RTL_H__
#define __RTL_RTL_H__

#include <cpu/decode.h>

#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)

#define dsrc1 (id_src1->preg)
#define dsrc2 (id_src2->preg)
#define ddest (id_dest->preg)
#define s0    (&s->tmp_reg[0])
#define s1    (&s->tmp_reg[1])
#define s2    (&s->tmp_reg[2])
#define t0    (&s->tmp_reg[3])

extern const rtlreg_t rzero;
#define rz (&rzero)

#define def_rtl(name, ...) void concat(rtl_, name)(DecodeExecState *s, __VA_ARGS__)

void rtl_exit(int state, vaddr_t halt_pc, uint32_t halt_ret);

// relation operation
enum {
  //            +-- unsign
  //            |   +-- sign
  //            |   |   +-- equal
  //            |   |   |   +-- invert
  //            |   |   |   |
  RELOP_FALSE = 0 | 0 | 0 | 0,
  RELOP_TRUE  = 0 | 0 | 0 | 1,
  RELOP_EQ    = 0 | 0 | 2 | 0,
  RELOP_NE    = 0 | 0 | 2 | 1,

  RELOP_LT    = 0 | 4 | 0 | 0,
  RELOP_LE    = 0 | 4 | 2 | 0,
  RELOP_GT    = 0 | 4 | 2 | 1,
  RELOP_GE    = 0 | 4 | 0 | 1,

  RELOP_LTU   = 8 | 0 | 0 | 0,
  RELOP_LEU   = 8 | 0 | 2 | 0,
  RELOP_GTU   = 8 | 0 | 2 | 1,
  RELOP_GEU   = 8 | 0 | 0 | 1,
};

#include <rtl-basic.h>
#include <rtl/pseudo.h>

#endif
