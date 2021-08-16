.section .rodata
.globl bin_start, bin_end
bin_start:
#ifdef BIN_PATH
.incbin BIN_PATH
#endif
bin_end:
