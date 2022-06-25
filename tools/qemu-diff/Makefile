#***************************************************************************************
# Copyright (c) 2014-2022 Zihao Yu, Nanjing University
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/

NAME  = $(GUEST_ISA)-qemu
SRCS  = $(shell find src/ -name "*.c")

SHARE = 1
CFLAGS += -DNEMU_HOME=\"$(NEMU_HOME)\" -DCONFIG_ISA_$(GUEST_ISA)
INC_PATH += $(NEMU_HOME)/include

include $(NEMU_HOME)/scripts/build.mk
