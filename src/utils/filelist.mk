ifdef CONFIG_ITRACE
CXXSRC = src/utils/disasm.cc
LLVM_VERSION=$(shell  llvm-config --version 2>/dev/null || echo "Not found")
ifeq ($(LLVM_VERSION),13.0.0)
        LLVM_CONFIG = llvm-config
else
        LLVM_CONFIG = llvm-config-11
endif
T1 = $(echo LLVM_VERSION)
T2 = $(echo LLVM_CONFIG)
CXXFLAGS += $(shell $(LLVM_CONFIG) --cxxflags) -fPIE
LIBS += $(shell $(LLVM_CONFIG) --libs)
endif
