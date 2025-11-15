/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __MEMORY_HOST_H__
#define __MEMORY_HOST_H__

#include <common.h>

static inline word_t host_read(void *addr, int len) {
  word_t ret = 0; 
  switch (len) {
    case 1: case 2: case 4: IFDEF(CONFIG_ISA64, case 8: )
    memcpy(&ret, addr, len); break;
    default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
  }
  return ret;
}

static inline void host_write(void *addr, int len, word_t data) {
  switch (len) {
    case 1:  case 2: case 4: IFDEF(CONFIG_ISA64, case 8: )
    memcpy(addr, &data, len); return;
    default: IFDEF(CONFIG_RT_CHECK,  assert(0));
  }
}

#endif
