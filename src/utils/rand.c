#include <common.h>
#ifndef CONFIG_TARGET_AM
#include <time.h>
#endif

void init_rand() {
  srand(MUXDEF(CONFIG_TARGET_AM, 0, time(0)));
}
