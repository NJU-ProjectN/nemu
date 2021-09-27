NAME  = x86-kvm
SRCS  = $(shell find src/ -name "*.c")

SHARE = 1
INC_PATH += $(NEMU_HOME)/include $(NEMU_HOME)/src/isa/x86/include
GUEST_ISA = x86

include $(NEMU_HOME)/scripts/build.mk
