NAME = nemu
INC_DIR += ./include
BUILD_DIR ?= ./build

ifeq ($(SHARE), 1)
SO = -so
SO_CFLAGS = -fPIC -D_SHARE=1
SO_LDLAGS = -shared -fPIC
endif

OBJ_DIR ?= $(BUILD_DIR)/obj$(SO)
BINARY ?= $(BUILD_DIR)/$(NAME)$(SO)

include Makefile.git

.DEFAULT_GOAL = app

# Compilation flags
CC = gcc
LD = gcc
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -MMD -Wall -Werror -ggdb3 $(INCLUDES) -fomit-frame-pointer
CFLAGS   += -DDIFF_TEST_QEMU

# Files to be compiled
SRCS = $(shell find src/ -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(SO_CFLAGS) -c -o $@ $<

# Uncomment the below assignment for test
# TESTCASE=dummy

ifdef TESTCASE
TEST_IMAGE=$(AM_HOME)/tests/cputest/build/$(TESTCASE)-x86-nemu.bin

# Command to build testcase
$(TEST_IMAGE): $(abspath $(TEST_IMAGE)/../..)/tests/$(TESTCASE).c
	cd $(abspath $(@D)/..) && make ARCH=x86-nemu ALL=$(TESTCASE)
endif

# Qemu dynamic linked libraries
QEMU_SO=$(NEMU_HOME)/tools/qemu-diff/build/qemu-so

# Command to build qemu-so
$(QEMU_SO): $(wildcard $(abspath $(QEMU_SO)/../../src)/*.c)
	cd $(abspath $(@D)/..) && make


# Depencies
-include $(OBJS:.o=.d)

# Some convenient rules

.PHONY: app run gdb clean run-env
app: $(BINARY)

override ARGS ?= -l $(BUILD_DIR)/nemu-log.txt
override ARGS += -d $(QEMU_SO)

# Command to execute NEMU
NEMU_EXEC := $(BINARY) $(ARGS) $(TEST_IMAGE)


$(BINARY): $(OBJS)
	$(call git_commit, "compile")
	@echo + LD $@
	@$(LD) -O2 -rdynamic $(SO_LDLAGS) -o $@ $^ -lSDL2 -lreadline -ldl

run-env: $(BINARY) $(TEST_IMAGE) $(QEMU_SO)

run: run-env
	$(call git_commit, "run")
	$(NEMU_EXEC)

gdb: run-env
	$(call git_commit, "gdb")
	gdb -s $(BINARY) --args $(NEMU_EXEC)

clean: 
	-rm -rf $(BUILD_DIR)
	$(MAKE) -C tools/gen-expr clean
	$(MAKE) -C tools/qemu-diff clean
