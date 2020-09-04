ifneq ($(MAKECMDGOALS),clean) # ignore check for make clean
ISA ?= x86

ifneq ($(ISA),x86) # ISA must be valid
$(error Only support x86)
endif
endif

INC_DIR += ./include $(NEMU_HOME)/include
BUILD_DIR ?= ./build
OBJ_DIR ?= $(BUILD_DIR)/obj-$(ISA)
BINARY ?= $(BUILD_DIR)/$(ISA)-kvm-so

.DEFAULT_GOAL = app

# Compilation flags
CC = gcc
LD = gcc
INCLUDES  = $(addprefix -I, $(INC_DIR))
CFLAGS   += -O2 -fPIC -MMD -Wall -Werror -DNEMU_HOME=$(NEMU_HOME) $(INCLUDES)

# Files to be compiled
SRCS = $(shell find src/ -name "*.c")
OBJS = $(SRCS:src/%.c=$(OBJ_DIR)/%.o)

# Compilation patterns
$(OBJ_DIR)/%.o: src/%.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS)  -c -o $@ $<


# Depencies
-include $(OBJS:.o=.d)

# Some convinient rules

.PHONY: app clean
app: $(BINARY)

$(BINARY): $(OBJS)
	@echo + LD $@
	@$(LD) -O2 -rdynamic -shared -fPIC -o $@ $^

clean: 
	-rm -rf $(BUILD_DIR)
