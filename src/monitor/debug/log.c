#include <common.h>
#include <stdarg.h>

FILE *log_fp = NULL;

void init_log(const char *log_file) {
  if (log_file == NULL) return;
  log_fp = fopen(log_file, "w");
  Assert(log_fp, "Can not open '%s'", log_file);
}

char log_bytebuf[80] = {};
char log_asmbuf[80] = {};
static char tempbuf[256] = {};

void strcatf(char *buf, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tempbuf, sizeof(tempbuf), fmt, ap);
  va_end(ap);
  strcat(buf, tempbuf);
}

void asm_print(vaddr_t this_pc, int instr_len, bool print_flag) {
  snprintf(tempbuf, sizeof(tempbuf), FMT_WORD ":   %s%*.s%s", this_pc, log_bytebuf,
      50 - (12 + 3 * instr_len), "", log_asmbuf);
  log_write("%s\n", tempbuf);
  if (print_flag) {
    puts(tempbuf);
  }

  log_bytebuf[0] = '\0';
  log_asmbuf[0] = '\0';
}
