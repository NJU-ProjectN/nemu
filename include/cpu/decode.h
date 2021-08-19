#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

#define OP_STR_SIZE 40

typedef struct {
  union {
    IFDEF(CONFIG_ISA_x86, uint64_t *pfreg);
    IFDEF(CONFIG_ISA_x86, uint64_t fval);
    rtlreg_t *preg;
    word_t imm;
    sword_t simm;
  };
  IFDEF(CONFIG_ISA_x86, rtlreg_t val);
  IFDEF(CONFIG_ISA_x86, uint8_t type);
  IFDEF(CONFIG_ISA_x86, uint8_t reg);
  IFDEF(CONFIG_DEBUG, char str[OP_STR_SIZE]);
} Operand;

typedef struct Decode {
  vaddr_t pc;
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  void (*EHelper)(struct Decode *);
  Operand dest, src1, src2;
  ISADecodeInfo isa;
  IFDEF(CONFIG_DEBUG, char logbuf[80]);
} Decode;

#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)


// --- instruction tracing log ---
#define print_Dop(...) IFDEF(CONFIG_DEBUG, snprintf(__VA_ARGS__))
#define print_asm(...) IFDEF(CONFIG_DEBUG, snprintf(log_asmbuf, sizeof(log_asmbuf), __VA_ARGS__))

#ifndef suffix_char
#define suffix_char(width) ' '
#endif

#define print_asm_template0(instr) \
  print_asm(str(instr) "%c", suffix_char(id_dest->width))

#define print_asm_template1(instr) \
  print_asm(str(instr) "%c %s", suffix_char(id_dest->width), id_dest->str)

#define print_asm_template2(instr) \
  print_asm(str(instr) "%c %s,%s", suffix_char(id_dest->width), id_src1->str, id_dest->str)

#define print_asm_template3(instr) \
  print_asm(str(instr) "%c %s,%s,%s", suffix_char(id_dest->width), id_src1->str, id_src2->str, id_dest->str)


// --- container for all instrucitons ---
#define INSTR_LIST(f) INSTR_NULLARY(f) INSTR_UNARY(f) INSTR_BINARY(f) INSTR_TERNARY(f)

#define def_EXEC_ID(name) \
  enum { concat(EXEC_ID_, name) = __COUNTER__ };
#define def_all_EXEC_ID() MAP(INSTR_LIST, def_EXEC_ID)

#define INSTR_CNT(name) + 1
#define TOTAL_INSTR (0 MAP(INSTR_LIST, INSTR_CNT))


// --- prototype of table helpers ---
#define def_THelper(name) static inline int concat(table_, name) (Decode *s)
#define def_THelper_arity(name, arity) \
  def_THelper(name) { concat(print_asm_template, arity)(name); return concat(EXEC_ID_, name); }
#define def_THelper_nullary(name) def_THelper_arity(name, 0)
#define def_THelper_unary(name)   def_THelper_arity(name, 1)
#define def_THelper_binary(name)  def_THelper_arity(name, 2)
#define def_THelper_ternary(name) def_THelper_arity(name, 3)

#define def_all_THelper() \
  MAP(INSTR_NULLARY, def_THelper_nullary) \
  MAP(INSTR_UNARY,   def_THelper_unary  ) \
  MAP(INSTR_BINARY,  def_THelper_binary ) \
  MAP(INSTR_TERNARY, def_THelper_ternary)


// --- prototype of decode helpers ---
#define def_DHelper(name) void concat(decode_, name) (Decode *s, int width)
// empty decode helper
static inline def_DHelper(empty) {}


// --- pattern matching mechanism ---
__attribute__((always_inline))
static inline void pattern_decode(const char *str, int len,
    uint32_t *key, uint32_t *mask, uint32_t *shift) {
  uint32_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }

#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint32_t *key, uint32_t *mask, uint32_t *shift) {
  uint32_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}


// --- pattern matching wrappers for decode ---
#define def_INSTR_raw(decode_fun, pattern, body) do { \
  uint32_t key, mask, shift; \
  decode_fun(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if (((get_instr(s) >> shift) & mask) == key) { body; } \
} while (0)

#define def_INSTR_IDTABW(pattern, id, tab, width) \
  def_INSTR_raw(pattern_decode, pattern, \
      { concat(decode_, id)(s, width); return concat(table_, tab)(s); })
#define def_INSTR_IDTAB(pattern, id, tab)   def_INSTR_IDTABW(pattern, id, tab, 0)
#define def_INSTR_TABW(pattern, tab, width) def_INSTR_IDTABW(pattern, empty, tab, width)
#define def_INSTR_TAB(pattern, tab)         def_INSTR_IDTABW(pattern, empty, tab, 0)

#define def_hex_INSTR_IDTABW(pattern, id, tab, width) \
  def_INSTR_raw(pattern_decode_hex, pattern, \
      { concat(decode_, id)(s, width); return concat(table_, tab)(s); })
#define def_hex_INSTR_IDTAB(pattern, id, tab)   def_hex_INSTR_IDTABW(pattern, id, tab, 0)
#define def_hex_INSTR_TABW(pattern, tab, width) def_hex_INSTR_IDTABW(pattern, empty, tab, width)
#define def_hex_INSTR_TAB(pattern, tab)         def_hex_INSTR_IDTABW(pattern, empty, tab, 0)

#endif
