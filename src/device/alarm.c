#include <common.h>
#include <sys/time.h>
#include <signal.h>

#define TIMER_HZ 60
#define MAX_HANDLER 8

typedef void (*alarm_handler_t) ();
static alarm_handler_t handler[MAX_HANDLER] = {};
static int idx = 0;
static uint32_t jiffy = 0;

void add_alarm_handle(void *h) {
  assert(idx < MAX_HANDLER);
  handler[idx ++] = h;
}

uint32_t uptime() { return jiffy / TIMER_HZ; }

static void alarm_sig_handler(int signum) {
  int i;
  for (i = 0; i < idx; i ++) {
    handler[i]();
  }

  jiffy ++;
}

void init_alarm() {
  struct sigaction s;
  memset(&s, 0, sizeof(s));
  s.sa_handler = alarm_sig_handler;
  int ret = sigaction(SIGVTALRM, &s, NULL);
  Assert(ret == 0, "Can not set signal handler");

  struct itimerval it = {};
  it.it_value.tv_sec = 0;
  it.it_value.tv_usec = 1000000 / TIMER_HZ;
  it.it_interval = it.it_value;
  ret = setitimer(ITIMER_VIRTUAL, &it, NULL);
  Assert(ret == 0, "Can not set timer");
}
