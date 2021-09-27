// from NEMU
#include <memory/paddr.h>
#include <isa-def.h>
#include <difftest-def.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

/* CR0 bits */
#define CR0_PE 1u
#define CR0_PG (1u << 31)

#define RFLAGS_ID  (1u << 21)
#define RFLAGS_AC  (1u << 18)
#define RFLAGS_RF  (1u << 16)
#define RFLAGS_TF  (1u << 8)
#define RFLAGS_AF  (1u << 4)
#define RFLAGS_FIX_MASK (RFLAGS_ID | RFLAGS_AC | RFLAGS_RF | RFLAGS_TF | RFLAGS_AF)

struct vm {
  int sys_fd;
  int fd;
  uint8_t *mem;
  uint8_t *mmio;
};

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
  int int_wp_state;
  int has_error_code;
  uint32_t entry;
};

enum {
  STATE_IDLE,      // if encounter an int instruction, then set watchpoint
  STATE_INT_INSTR, // if hit the watchpoint, then delete the watchpoint
  STATE_IRET_INSTR,// if hit the watchpoint, then delete the watchpoint
};

static struct vm vm;
static struct vcpu vcpu;

// This should be called everytime after KVM_SET_REGS.
// It seems that KVM_SET_REGS will clean the state of single step.
static void kvm_set_step_mode(bool watch, uint32_t watch_addr) {
  struct kvm_guest_debug debug = {};
  debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP | KVM_GUESTDBG_USE_HW_BP;
  debug.arch.debugreg[0] = watch_addr;
  debug.arch.debugreg[7] = (watch ? 0x1 : 0x0); // watch instruction fetch at `watch_addr`
  if (ioctl(vcpu.fd, KVM_SET_GUEST_DEBUG, &debug) < 0) {
    perror("KVM_SET_GUEST_DEBUG");
    assert(0);
  }
}

static void kvm_setregs(const struct kvm_regs *r) {
  if (ioctl(vcpu.fd, KVM_SET_REGS, r) < 0) {
    perror("KVM_SET_REGS");
    assert(0);
  }
  kvm_set_step_mode(false, 0);
}

static void kvm_getsregs(struct kvm_sregs *r) {
  if (ioctl(vcpu.fd, KVM_GET_SREGS, r) < 0) {
    perror("KVM_GET_SREGS");
    assert(0);
  }
}

static void kvm_setsregs(const struct kvm_sregs *r) {
  if (ioctl(vcpu.fd, KVM_SET_SREGS, r) < 0) {
    perror("KVM_SET_SREGS");
    assert(0);
  }
}

static void* create_mem(int slot, uintptr_t base, size_t mem_size) {
  void *mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (mem == MAP_FAILED) {
    perror("mmap mem");
    assert(0);
  }

  madvise(mem, mem_size, MADV_MERGEABLE);

  struct kvm_userspace_memory_region memreg;
  memreg.slot = slot;
  memreg.flags = 0;
  memreg.guest_phys_addr = base;
  memreg.memory_size = mem_size;
  memreg.userspace_addr = (unsigned long)mem;
  if (ioctl(vm.fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    assert(0);
  }
  return mem;
}

static void vm_init(size_t mem_size) {
  int api_ver;

  vm.sys_fd = open("/dev/kvm", O_RDWR);
  if (vm.sys_fd < 0) {
    perror("open /dev/kvm");
    assert(0);
  }

  api_ver = ioctl(vm.sys_fd, KVM_GET_API_VERSION, 0);
  if (api_ver < 0) {
    perror("KVM_GET_API_VERSION");
    assert(0);
  }

  if (api_ver != KVM_API_VERSION) {
    fprintf(stderr, "Got KVM api version %d, expected %d\n",
        api_ver, KVM_API_VERSION);
    assert(0);
  }

  vm.fd = ioctl(vm.sys_fd, KVM_CREATE_VM, 0);
  if (vm.fd < 0) {
    perror("KVM_CREATE_VM");
    assert(0);
  }

  if (ioctl(vm.fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
    perror("KVM_SET_TSS_ADDR");
    assert(0);
  }

  vm.mem = create_mem(0, 0, mem_size);
  vm.mmio = create_mem(1, 0xa1000000, 0x1000);
}

static void vcpu_init() {
  int vcpu_mmap_size;

  vcpu.fd = ioctl(vm.fd, KVM_CREATE_VCPU, 0);
  if (vcpu.fd < 0) {
    perror("KVM_CREATE_VCPU");
    assert(0);
  }

  vcpu_mmap_size = ioctl(vm.sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if (vcpu_mmap_size <= 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    assert(0);
  }

  vcpu.kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
      MAP_SHARED, vcpu.fd, 0);
  if (vcpu.kvm_run == MAP_FAILED) {
    perror("mmap kvm_run");
    assert(0);
  }

  vcpu.kvm_run->kvm_valid_regs = KVM_SYNC_X86_REGS | KVM_SYNC_X86_SREGS;
  vcpu.int_wp_state = STATE_IDLE;
}

static const uint8_t mbr[] = {
  // start32:
  0x0f, 0x01, 0x15, 0x28, 0x7c, 0x00, 0x00,  // lgdtl 0x7c28
  0xea, 0x0e, 0x7c, 0x00, 0x00, 0x08, 0x00,  // ljmp $0x8, 0x7c0e

  // here:
  0xeb, 0xfe,  // jmp here

  // GDT
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00,

  // GDT descriptor
  0x17, 0x00, 0x10, 0x7c, 0x00, 0x00
};

static void setup_protected_mode(struct kvm_sregs *sregs) {
  struct kvm_segment seg = {
    .base = 0,
    .limit = 0xffffffff,
    .selector = 1 << 3,
    .present = 1,
    .type = 11, /* Code: execute, read, accessed */
    .dpl = 0,
    .db = 1,
    .s = 1, /* Code/data */
    .l = 0,
    .g = 1, /* 4KB granularity */
  };

  sregs->cr0 |= CR0_PE; /* enter protected mode */

  sregs->cs = seg;

  seg.type = 3; /* Data: read/write, accessed */
  seg.selector = 2 << 3;
  sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static uint64_t va2pa(uint64_t va) {
  if (vcpu.kvm_run->s.regs.sregs.cr0 & CR0_PG) {
    struct kvm_translation t = { .linear_address = va };
    int ret = ioctl(vcpu.fd, KVM_TRANSLATE, &t);
    assert(ret == 0);
    return t.valid ? t.physical_address : -1ull;
  }
  return va;
}

static int patching() {
  // patching for special instructions
  uint32_t pc = va2pa(vcpu.kvm_run->s.regs.regs.rip);
  if (pc == 0xffffffff) return 0;
  if (vm.mem[pc] == 0x9c) {  // pushf
    if (vcpu.int_wp_state == STATE_INT_INSTR) return 0;
    vcpu.kvm_run->s.regs.regs.rsp -= 4;
    uint32_t esp = va2pa(vcpu.kvm_run->s.regs.regs.rsp);
    *(uint32_t *)(vm.mem + esp) = vcpu.kvm_run->s.regs.regs.rflags & ~RFLAGS_FIX_MASK;
    vcpu.kvm_run->s.regs.regs.rflags |= RFLAGS_TF;
    vcpu.kvm_run->s.regs.regs.rip ++;
    vcpu.kvm_run->kvm_dirty_regs = KVM_SYNC_X86_REGS;
    return 1;
  }
  else if (vm.mem[pc] == 0x9d) {  // popf
    if (vcpu.int_wp_state == STATE_INT_INSTR) return 0;
    uint32_t esp = va2pa(vcpu.kvm_run->s.regs.regs.rsp);
    vcpu.kvm_run->s.regs.regs.rflags = *(uint32_t *)(vm.mem + esp) | RFLAGS_TF | 2;
    vcpu.kvm_run->s.regs.regs.rsp += 4;
    vcpu.kvm_run->s.regs.regs.rip ++;
    vcpu.kvm_run->kvm_dirty_regs = KVM_SYNC_X86_REGS;
    return 1;
  }
  else if (vm.mem[pc] == 0xcf) { // iret
    uint32_t ret_addr = va2pa(vcpu.kvm_run->s.regs.regs.rsp);
    uint32_t eip = *(uint32_t *)(vm.mem + ret_addr);
    vcpu.entry = eip;
    kvm_set_step_mode(true, eip);
    vcpu.int_wp_state = STATE_IRET_INSTR;
    return 0;
  }
  return 0;
}

static void fix_push_sreg() {
  uint32_t esp = va2pa(vcpu.kvm_run->s.regs.regs.rsp);
  *(uint32_t *)(vm.mem + esp) &= 0x0000ffff;
}

static void patching_after(uint64_t last_pc) {
  uint32_t pc = va2pa(last_pc);
  if (pc == 0xffffffff) return;
  uint8_t opcode = vm.mem[pc];
  if (opcode == 0x1e || opcode == 0x06) {  // push %ds/%es
    fix_push_sreg();
    assert(vcpu.kvm_run->s.regs.regs.rip == last_pc + 1);
  }
  else if (opcode == 0x0f) {
    uint8_t opcode2 = vm.mem[pc + 1];
    if (opcode2 == 0xa0) { // push %fs
      fix_push_sreg();
      assert(vcpu.kvm_run->s.regs.regs.rip == last_pc + 2);
    }
  }
}

static void kvm_exec(uint64_t n) {
  for (; n > 0; n --) {
    if (patching()) continue;

    uint64_t pc = vcpu.kvm_run->s.regs.regs.rip;
    if (ioctl(vcpu.fd, KVM_RUN, 0) < 0) {
      if (errno == EINTR) {
        n ++;
        continue;
      }
      perror("KVM_RUN");
      assert(0);
    }

    if (vcpu.kvm_run->exit_reason != KVM_EXIT_DEBUG) {
      if (vcpu.kvm_run->exit_reason == KVM_EXIT_HLT) return;
      fprintf(stderr,	"Got exit_reason %d at pc = 0x%llx, expected KVM_EXIT_DEBUG (%d)\n",
          vcpu.kvm_run->exit_reason, vcpu.kvm_run->s.regs.regs.rip, KVM_EXIT_DEBUG);
      assert(0);
    } else {
      patching_after(pc);
      if (vcpu.int_wp_state == STATE_INT_INSTR) {
        uint32_t eflag_offset = 8 + (vcpu.has_error_code ? 4 : 0);
        uint32_t eflag_addr = va2pa(vcpu.kvm_run->s.regs.regs.rsp + eflag_offset);
        *(uint32_t *)(vm.mem + eflag_addr) &= ~RFLAGS_FIX_MASK;

        Assert(vcpu.entry == vcpu.kvm_run->debug.arch.pc,
            "entry not match, right = 0x%llx, wrong = 0x%x", vcpu.kvm_run->debug.arch.pc, vcpu.entry);
        kvm_set_step_mode(false, 0);
        vcpu.int_wp_state = STATE_IDLE;
      //Log("exception = %d, pc = %llx, dr6 = %llx, dr7 = %llx", vcpu.kvm_run->debug.arch.exception,
      //    vcpu.kvm_run->debug.arch.pc, vcpu.kvm_run->debug.arch.dr6, vcpu.kvm_run->debug.arch.dr7);
      } else if (vcpu.int_wp_state == STATE_IRET_INSTR) {
        Assert(vcpu.entry == vcpu.kvm_run->debug.arch.pc,
            "entry not match, right = 0x%llx, wrong = 0x%x", vcpu.kvm_run->debug.arch.pc, vcpu.entry);
        kvm_set_step_mode(false, 0);
        vcpu.int_wp_state = STATE_IDLE;
      }
    }
  }
}

static void run_protected_mode() {
  struct kvm_sregs sregs;
  kvm_getsregs(&sregs);
  setup_protected_mode(&sregs);
  kvm_setsregs(&sregs);

  struct kvm_regs regs;
  memset(&regs, 0, sizeof(regs));
  regs.rflags = 2;
  regs.rip = 0x7c00;
  // this will also set KVM_GUESTDBG_ENABLE
  kvm_setregs(&regs);

  memcpy(vm.mem + 0x7c00, mbr, sizeof(mbr));
  // run enough instructions to load GDT
  kvm_exec(10);
}

void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if (direction == DIFFTEST_TO_REF) memcpy(vm.mem + addr, buf, n);
  else memcpy(buf, vm.mem + addr, n);
}

void difftest_regcpy(void *r, bool direction) {
  struct kvm_regs *ref = &(vcpu.kvm_run->s.regs.regs);
  x86_CPU_state *x86 = r;
  if (direction == DIFFTEST_TO_REF) {
    ref->rax = x86->eax;
    ref->rbx = x86->ebx;
    ref->rcx = x86->ecx;
    ref->rdx = x86->edx;
    ref->rsp = x86->esp;
    ref->rbp = x86->ebp;
    ref->rsi = x86->esi;
    ref->rdi = x86->edi;
    ref->rip = x86->pc;
    ref->rflags |= RFLAGS_TF;
    vcpu.kvm_run->kvm_dirty_regs = KVM_SYNC_X86_REGS;
  } else {
    x86->eax = ref->rax;
    x86->ebx = ref->rbx;
    x86->ecx = ref->rcx;
    x86->edx = ref->rdx;
    x86->esp = ref->rsp;
    x86->ebp = ref->rbp;
    x86->esi = ref->rsi;
    x86->edi = ref->rdi;
    x86->pc  = ref->rip;
  }
}

void difftest_exec(uint64_t n) {
  kvm_exec(n);
}

void difftest_raise_intr(word_t NO) {
  uint32_t pgate_vaddr = vcpu.kvm_run->s.regs.sregs.idt.base + NO * 8;
  uint32_t pgate = va2pa(pgate_vaddr);
  // assume code.base = 0
  uint32_t entry = vm.mem[pgate] | (vm.mem[pgate + 1] << 8) |
    (vm.mem[pgate + 6] << 16) | (vm.mem[pgate + 7] << 24);
  kvm_set_step_mode(true, entry);
  vcpu.int_wp_state = STATE_INT_INSTR;
  vcpu.has_error_code = (NO == 14);
  vcpu.entry = entry;

  if (NO == 48) {
    // inject timer interrupt
    struct kvm_interrupt intr = { .irq = NO };
    int ret = ioctl(vcpu.fd, KVM_INTERRUPT, &intr);
    assert(ret == 0);
  }
}

void difftest_init(int port) {
  vm_init(CONFIG_MSIZE);
  vcpu_init();
  run_protected_mode();
}
