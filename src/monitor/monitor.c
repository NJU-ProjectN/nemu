#include "nemu.h"

#define ENTRY_START 0x100000

void init_check();
void init_regex();
void init_wp_pool();
void init_device();

void reg_test();
void init_qemu_reg();
bool gdb_memcpy_to_qemu(uint32_t, void *, int);

FILE *log_fp = NULL;

static inline void init_log() {
  log_fp = fopen("log.txt", "w");
  Assert(log_fp, "Can not open 'log.txt'");
}

static inline void welcome() {
  printf("Welcome to NEMU!\n");
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("For help, type \"help\"\n");
}

static inline void load_default_img() {
  const uint8_t img []  = {
    0xb8, 0x34, 0x12, 0x00, 0x00,        // 100000:  movl  $0x1234,%eax
    0xb9, 0x27, 0x00, 0x10, 0x00,        // 100005:  movl  $0x100027,%ecx
    0x89, 0x01,                          // 10000a:  movl  %eax,(%ecx)
    0x66, 0xc7, 0x41, 0x04, 0x01, 0x00,  // 10000c:  movw  $0x1,0x4(%ecx)
    0xbb, 0x02, 0x00, 0x00, 0x00,        // 100012:  movl  $0x2,%ebx
    0x66, 0xc7, 0x84, 0x99, 0x00, 0xe0,  // 100017:  movw  $0x1,-0x2000(%ecx,%ebx,4)
    0xff, 0xff, 0x01, 0x00,
    0xb8, 0x00, 0x00, 0x00, 0x00,        // 100021:  movl  $0x0,%eax
    0xd6,                                // 100026:  nemu_trap
  };

  printf("No image is given. Use the default build-in image.\n");

  memcpy(guest_to_host(ENTRY_START), img, sizeof(img));
#ifdef CROSS_CHECK
  gdb_memcpy_to_qemu(ENTRY_START, guest_to_host(ENTRY_START), sizeof(img));
#endif
}

static inline void load_img(int argc, char *argv[]) {
  if (argc == 1) {
    load_default_img();
    return;
  }

  int ret;
  Assert(argc <= 2, "run NEMU with format 'nemu program [disk_img]'");
  char *exec_file = argv[1];

  FILE *fp = fopen(exec_file, "rb");
  Assert(fp, "Can not open '%s'", exec_file);

  printf("The image is %s\n", exec_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  fseek(fp, 0, SEEK_SET);
  ret = fread(guest_to_host(ENTRY_START), size, 1, fp);
  assert(ret == 1);

  fclose(fp);

#ifdef CROSS_CHECK
  gdb_memcpy_to_qemu(ENTRY_START, guest_to_host(ENTRY_START), size);
#endif
}

static inline void restart() {
  /* Set the initial instruction pointer. */
  cpu.eip = ENTRY_START;

#ifdef CROSS_CHECK
  init_qemu_reg();
#endif
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

#ifdef LOG_FILE
  /* Open the log file. */
  init_log();
#endif

  /* Test the implementation of the `CPU_state' structure. */
  reg_test();

#ifdef CROSS_CHECK
  /* Fork a child process to perform cross checking. */
  init_check();
#endif

  /* Load the image to memory. */
  load_img(argc, argv);

  /* Initialize this virtual computer system. */
  restart();

  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Initialize devices. */
  init_device();

  /* Display welcome message. */
  welcome();
}
