ifdef CONFIG_ITRACE
CXXSRC = src/utils/disasm.cc
CXXFLAGS += $(shell llvm-config-11 --cxxflags) -fPIE
LIBS += $(shell llvm-config-11 --libs)
endif
