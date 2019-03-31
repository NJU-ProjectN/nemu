#include "nemu.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 10

NEMUState nemu_state = {.state = NEMU_STOP};

void interpret_rtl_exit(int state, vaddr_t halt_pc, uint32_t halt_ret) {
  nemu_state = (NEMUState) { .state = state, .halt_pc = halt_pc, .halt_ret = halt_ret };
}

void exec_wrapper(bool);

static uint64_t g_nr_guest_instr = 0;

void nr_guest_instr_add(uint32_t n) {
  g_nr_guest_instr += n;
}

void monitor_statistic() {
  Log("total guest instructions = %ld", g_nr_guest_instr);
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  if (nemu_state.state == NEMU_END || nemu_state.state == NEMU_ABORT) {
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  }
  nemu_state.state = NEMU_RUNNING;

  bool print_flag = n < MAX_INSTR_TO_PRINT;

  for (; n > 0; n --) {
    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
    exec_wrapper(print_flag);
    nr_guest_instr_add(1);

#ifdef DEBUG
    /* TODO: check watchpoints here. */

#endif

#ifdef HAS_IOE
    extern void device_update();
    device_update();
#endif

    if (nemu_state.state != NEMU_RUNNING) {
      if (nemu_state.state == NEMU_END || nemu_state.state == NEMU_ABORT) {
        printflog("nemu: %s\33[0m at pc = 0x%08x\n\n",
            (nemu_state.state == NEMU_ABORT ? "\33[1;31mABORT" :
             (nemu_state.halt_ret == 0 ? "\33[1;32mHIT GOOD TRAP" : "\33[1;31mHIT BAD TRAP")),
            nemu_state.halt_pc);
        monitor_statistic();
        return;
      }
    }
  }

  if (nemu_state.state == NEMU_RUNNING) { nemu_state.state = NEMU_STOP; }
}
