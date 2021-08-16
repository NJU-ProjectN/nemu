#include <rtl/rtl.h>

void isa_hostcall(uint32_t id, rtlreg_t *dest,
    const rtlreg_t *src1, const rtlreg_t *src2, word_t imm) {
  word_t ret = 0;
  switch (id) {
    default: panic("Unsupported hostcall ID = %d", id);
  }
  if (dest) *dest = ret;
}
