/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs); } while (0)
#define src2R() do { *src2 = R(rt); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 15, 0), 16); } while(0)
#define immU() do { *imm = BITS(i, 15, 0); } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rt = BITS(i, 20, 16);
  int rs = BITS(i, 25, 21);
  *rd = (type == TYPE_U || type == TYPE_I) ? rt : BITS(i, 15, 11);
  switch (type) {
    case TYPE_I: src1R(); immI(); break;
    case TYPE_U: src1R(); immU(); break;
  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}

  INSTPAT_START();
  INSTPAT("001111 ????? ????? ????? ????? ??????", lui    , U, R(rd) = imm << 16);
  INSTPAT("100011 ????? ????? ????? ????? ??????", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("101011 ????? ????? ????? ????? ??????", sw     , I, Mw(src1 + imm, 4, R(rd)));

  INSTPAT("011100 ????? ????? ????? ????? 111111", sdbbp  , N, NEMUTRAP(s->pc, R(2))); // R(2) is $v0;
  INSTPAT("?????? ????? ????? ????? ????? ??????", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
