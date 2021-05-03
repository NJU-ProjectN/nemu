#include "common.h"
#include <sys/prctl.h>
#include <signal.h>
#include _ISA_H_

bool gdb_connect_qemu(int);
bool gdb_memcpy_to_qemu(uint32_t, void *, int);
bool gdb_getregs(union isa_gdb_regs *);
bool gdb_setregs(union isa_gdb_regs *);
bool gdb_si();
void gdb_exit();

void init_isa();

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  bool ok = gdb_memcpy_to_qemu(dest, src, n);
  assert(ok == 1);
}

void difftest_getregs(void *r) {
  union isa_gdb_regs qemu_r;
  gdb_getregs(&qemu_r);
  memcpy(r, &qemu_r, DIFFTEST_REG_SIZE);
}

void difftest_setregs(const void *r) {
  union isa_gdb_regs qemu_r;
  gdb_getregs(&qemu_r);
  memcpy(&qemu_r, r, DIFFTEST_REG_SIZE);
  gdb_setregs(&qemu_r);
}

void difftest_exec(uint64_t n) {
  while (n --) gdb_si();
}

void difftest_init(int port) {
  char buf[32];
  sprintf(buf, "tcp::%d", port);

  int ppid_before_fork = getpid();
  int pid = fork();
  if (pid == -1) {
    perror("fork");
    assert(0);
  }
  else if (pid == 0) {
    // child

    // install a parent death signal in the chlid
    int r = prctl(PR_SET_PDEATHSIG, SIGTERM);
    if (r == -1) {
      perror("prctl error");
      assert(0);
    }

    if (getppid() != ppid_before_fork) {
      printf("parent has died!\n");
      assert(0);
    }

    close(STDIN_FILENO);
    execlp(ISA_QEMU_BIN, ISA_QEMU_BIN, ISA_QEMU_ARGS "-S", "-gdb", buf, "-nographic", NULL);
    perror("exec");
    assert(0);
  }
  else {
    // father

    gdb_connect_qemu(port);
    printf("Connect to QEMU with %s successfully\n", buf);

    atexit(gdb_exit);

    init_isa();
  }
}
