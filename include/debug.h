#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <common.h>
#include <stdio.h>
#include <utils.h>

#define Log(format, ...) \
    _Log(ASNI_FMT("[%s:%d %s] " format, ASNI_FG_BLUE) "\n", \
        __FILE__, __LINE__, __func__, ## __VA_ARGS__)

#define Assert(cond, format, ...) \
  do { \
    if (!(cond)) { \
      MUXDEF(CONFIG_TARGET_AM, printf(ASNI_FMT(format, ASNI_FG_RED) "\n", ## __VA_ARGS__), \
        (fflush(stdout), fprintf(stderr, ASNI_FMT(format, ASNI_FG_RED) "\n", ##  __VA_ARGS__))); \
      extern void assert_fail_msg(); \
      assert_fail_msg(); \
      assert(cond); \
    } \
  } while (0)

#define panic(format, ...) Assert(0, format, ## __VA_ARGS__)

#define TODO() panic("please implement me")

#endif
