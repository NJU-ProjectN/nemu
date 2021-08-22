#include <utils.h>
#include <cpu/ifetch.h>
#include <rtl/rtl.h>
#include <cpu/difftest.h>

uint32_t pio_read(ioaddr_t addr, int len);
void pio_write(ioaddr_t addr, int len, uint32_t data);

void set_nemu_state(int state, vaddr_t pc, int halt_ret) {
  nemu_state.state = state;
  nemu_state.halt_pc = pc;
  nemu_state.halt_ret = halt_ret;
}

static void invalid_instr(vaddr_t thispc) {
  uint32_t temp[2];
  vaddr_t pc = thispc;
  temp[0] = instr_fetch(&pc, 4);
  temp[1] = instr_fetch(&pc, 4);

  uint8_t *p = (uint8_t *)temp;
  printf("invalid opcode(PC = " FMT_WORD "):\n"
      "\t%02x %02x %02x %02x %02x %02x %02x %02x ...\n"
      "\t%08x %08x...\n",
      thispc, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], temp[0], temp[1]);

  printf("There are two cases which will trigger this unexpected exception:\n"
      "1. The instruction at PC = " FMT_WORD " is not implemented.\n"
      "2. Something is implemented incorrectly.\n", thispc);
  printf("Find this PC(" FMT_WORD ") in the disassembling result to distinguish which case it is.\n\n", thispc);
  printf(ASNI_FMT("If it is the first case, see\n%s\nfor more details.\n\n"
        "If it is the second case, remember:\n"
        "* The machine is always right!\n"
        "* Every line of untested code is always wrong!\n\n", ASNI_FG_RED), isa_logo);

  set_nemu_state(NEMU_ABORT, thispc, -1);
}

def_rtl(hostcall, uint32_t id, rtlreg_t *dest, const rtlreg_t *src1,
    const rtlreg_t *src2, word_t imm) {
  switch (id) {
    case HOSTCALL_EXIT:
      difftest_skip_ref();
      set_nemu_state(NEMU_END, s->pc, *src1);
      break;
    case HOSTCALL_INV: invalid_instr(s->pc); break;
#ifdef CONFIG_HAS_PORT_IO
    case HOSTCALL_PIO: {
      int width = imm & 0xf;
      bool is_in = ((imm & ~0xf) != 0);
      if (is_in) *dest = pio_read(*src1, width);
      else pio_write(*dest, width, *src1);
      break;
    }
#endif
    default: panic("Unsupport hostcall ID = %d", id); break;
  }
}
