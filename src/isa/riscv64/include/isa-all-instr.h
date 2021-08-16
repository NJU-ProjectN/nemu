#include <cpu/decode.h>
#include "../local-include/rtl.h"

#define INSTR_NULLARY(f) f(inv) f(nemu_trap)
#define INSTR_UNARY(f)
#define INSTR_BINARY(f) f(auipc)
#define INSTR_TERNARY(f) f(ld) f(sd)

def_all_EXEC_ID();
