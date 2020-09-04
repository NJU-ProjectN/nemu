static inline def_EHelper(lui) {
  rtl_li(s, ddest, id_src2->imm << 16);
  print_asm_template3(lui);
}
