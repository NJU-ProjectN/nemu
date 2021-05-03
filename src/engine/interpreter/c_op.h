#ifndef __C_OP_H__
#define __C_OP_H__

#include <common.h>

#ifdef ISA64
#define c_shift_mask 0x3f
#else
#define c_shift_mask 0x1f
#endif

#define c_add(a, b) ((a) + (b))
#define c_sub(a, b) ((a) - (b))
#define c_and(a, b) ((a) & (b))
#define c_or(a, b)  ((a) | (b))
#define c_xor(a, b) ((a) ^ (b))
#define c_shl(a, b) ((a) << ((b) & c_shift_mask))
#define c_shr(a, b) ((a) >> ((b) & c_shift_mask))
#define c_sar(a, b) ((sword_t)(a) >> ((b) & c_shift_mask))

#ifdef ISA64
#define c_sext32to64(a) ((int64_t)(int32_t)(a))
#define c_addw(a, b) c_sext32to64((a) + (b))
#define c_subw(a, b) c_sext32to64((a) - (b))
#define c_shlw(a, b) c_sext32to64((uint32_t)(a) << ((b) & 0x1f))
#define c_shrw(a, b) c_sext32to64((uint32_t)(a) >> ((b) & 0x1f))
#define c_sarw(a, b) c_sext32to64(( int32_t)(a) >> ((b) & 0x1f))
#endif

#define c_mul_lo(a, b) ((a) * (b))
#define c_imul_lo(a, b) ((sword_t)(a) * (sword_t)(b))
#ifdef ISA64
# define c_mul_hi(a, b) (((__uint128_t)(a) * (__uint128_t)(b)) >> 64)
# define c_imul_hi(a, b) (((__int128_t)(sword_t)(a) * (__int128_t)(sword_t)(b)) >> 64)
# define c_mulw(a, b) c_sext32to64((a) * (b))
# define c_divw(a, b)  c_sext32to64(( int32_t)(a) / ( int32_t)(b))
# define c_divuw(a, b) c_sext32to64((uint32_t)(a) / (uint32_t)(b))
# define c_remw(a, b)  c_sext32to64(( int32_t)(a) % ( int32_t)(b))
# define c_remuw(a, b) c_sext32to64((uint32_t)(a) % (uint32_t)(b))
#else
#define c_mul_hi(a, b) (((uint64_t)(a) * (uint64_t)(b)) >> 32)
#define c_imul_hi(a, b) (((int64_t)(sword_t)(a) * (int64_t)(sword_t)(b)) >> 32)
#endif

#define c_div_q(a, b) ((a) / (b))
#define c_div_r(a, b)  ((a) % (b))
#define c_idiv_q(a, b) ((sword_t)(a) / (sword_t)(b))
#define c_idiv_r(a, b)  ((sword_t)(a) % (sword_t)(b))

static inline bool interpret_relop(uint32_t relop, const rtlreg_t src1, const rtlreg_t src2) {
  switch (relop) {
    case RELOP_FALSE: return false;
    case RELOP_TRUE: return true;
    case RELOP_EQ: return src1 == src2;
    case RELOP_NE: return src1 != src2;
    case RELOP_LT: return (sword_t)src1 <  (sword_t)src2;
    case RELOP_LE: return (sword_t)src1 <= (sword_t)src2;
    case RELOP_GT: return (sword_t)src1 >  (sword_t)src2;
    case RELOP_GE: return (sword_t)src1 >= (sword_t)src2;
    case RELOP_LTU: return src1 < src2;
    case RELOP_LEU: return src1 <= src2;
    case RELOP_GTU: return src1 > src2;
    case RELOP_GEU: return src1 >= src2;
    default: panic("unsupport relop = %d", relop);
  }
}

#endif
