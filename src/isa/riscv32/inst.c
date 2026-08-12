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

#include "local-include/reg.h"
#include "macro.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

#define INST_TYPE_LIST                                \
  X(N, _1  , _2  , _3, _4)      /* none            */ \
  X(I, vrs1, _2  , rd, imm)     /* Immediate       */ \
  X(U, _1  , _2  , rd, imm)     /* Upper Immediate */ \
  X(S, vrs1, vrs2, _3, imm)     /* Store           */

typedef enum {
#define X(T, _1, _2, _3, _4) concat(TYPE_, T),
  INST_TYPE_LIST
#undef X
} inst_type;

#define X(T, _1, _2, _3, _4) \
  typedef struct { word_t _1; word_t _2; word_t _3; word_t _4; } concat(decode_, T);
INST_TYPE_LIST
#undef X


#define D_1(T)
#define D_2(T)
#define D_3(T)
#define D_4(T)

#define Drs1(T) BITS(i, 19, 15)
#define Drs2(T) BITS(i, 24, 20)
#define Drd(T)  do { ret._3 = BITS(i, 11, 7); } while(0)

#define Dvrs1(T) do { ret._1 = R(Drs1(T)); } while (0)
#define Dvrs2(T) do { ret._2 = R(Drs2(T)); } while (0)

#define Dimm(T)  concat(Dimm, T)(T)
#define DimmI(T) do { ret._4 = SEXT(BITS(i, 31, 20), 12); } while(0)
#define DimmU(T) do { ret._4 = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define DimmS(T) do { ret._4 = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)

static decode_N decode_operand(Decode *s, inst_type type) {
  word_t i = s->isa.inst;
  decode_N ret = {0, 0, 0, 0};
  switch (type) {
#define X(T, _1, _2, _3, _4) \
  case concat(TYPE_, T):     \
    concat(D, _1)(T);        \
    concat(D, _2)(T);        \
    concat(D, _3)(T);        \
    concat(D, _4)(T);        \
    break;
    INST_TYPE_LIST
    /* For clarity, the INST_TYPE_LIST macro above expands the switch cases
     * into the following logical structure:
     *
     * case TYPE_N   : D_1(T)    ; D_2(T)   ; D_3(T) ; D_4(T)   ; break ;
     * case TYPE_I   : Dvrs1(T)  ; D_2(T)   ; Drd(T) ; DimmI(T) ; break ;
     * case TYPE_U   : D_1(T)    ; D_2(T)   ; Drd(T) ; DimmU(T) ; break ;
     * case TYPE_S   : Dvrs1(T)  ; Dvrs2(T) ; D_3(T) ; DimmS(T) ; break ;
     * ... and so on for other types.
     */
#undef X
    default: panic("unsupported type = %d", type);
  }
  return ret;
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, T, ... /* execute body */ ) { \
  decode_N temp = decode_operand(s, concat(TYPE_, T)); \
  [[maybe_unused]] concat(decode_, T) decode_result = BITCAST(concat(decode_, T), temp); \
  __VA_ARGS__ ; \
}

#define vrs1 decode_result.vrs1
#define vrs2 decode_result.vrs2
#define rd   decode_result.rd
#define imm  decode_result.imm

  INSTPAT_START();
  
  INSTPAT(" ??????? ????? ????? ??? ????? 0010111 "  , auipc      , U    , R(rd) = s->pc + imm);
  INSTPAT(" ??????? ????? ????? 100 ????? 0000011 "  , lbu        , I    , R(rd)=Mr(vrs1 + imm, 1));
  INSTPAT(" ??????? ????? ????? 000 ????? 0100011 "  , sb         , S    , Mw(vrs1 + imm,1,  BITS(vrs2, 7, 0)));

  INSTPAT(" 0000000 00001 00000 000 00000 1110011 "  , ebreak     , N    , NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT(" ??????? ????? ????? ??? ????? ??????? "  , inv        , N    , INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
