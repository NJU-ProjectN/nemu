#ifndef __CPU_IFETCH_H__

#include <memory/vaddr.h>

static inline uint32_t instr_fetch(vaddr_t *pc, int len) {
  uint32_t instr = vaddr_ifetch(*pc, len);
#ifdef CONFIG_DEBUG
  uint8_t *p_instr = (void *)&instr;
  int i;
  for (i = 0; i < len; i ++) {
    int l = strlen(log_bytebuf);
    snprintf(log_bytebuf + l, sizeof(log_bytebuf) - l, "%02x ", p_instr[i]);
  }
#endif
  (*pc) += len;
  return instr;
}

#endif
