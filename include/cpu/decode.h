#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

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
} Operand;

typedef struct Decode {
  vaddr_t pc;
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  void (*EHelper)(struct Decode *);
  Operand dest, src1, src2;
  ISADecodeInfo isa;
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;

#define id_src1 (&s->src1)
#define id_src2 (&s->src2)
#define id_dest (&s->dest)


// `INSTR_LIST` is defined at src/isa/$ISA/include/isa-all-instr.h
#define def_EXEC_ID(name) concat(EXEC_ID_, name),
#define def_all_EXEC_ID() enum { MAP(INSTR_LIST, def_EXEC_ID) TOTAL_INSTR }


// --- prototype of table helpers ---
#define def_THelper(name) static inline int concat(table_, name) (Decode *s)
#define def_THelper_body(name) def_THelper(name) { return concat(EXEC_ID_, name); }
#define def_all_THelper() MAP(INSTR_LIST, def_THelper_body)


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
