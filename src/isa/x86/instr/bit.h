uint32_t bit_scan(uint32_t x, bool reverse);

def_EHelper(bt) {
  if (id_dest->type == OP_TYPE_MEM) {
    rtl_srli(s, s0, dsrc1, 5);
    rtl_slli(s, s0, s0, 2);
    rtl_add(s, s0, s->isa.mbase, s0);
    s->isa.mbase = s0;
  }

  rtl_decode_binary(s, true, true);

  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s0, s0, ddest);

  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, s0, NULL, NULL, LAZYCC_BT, s->isa.width);
#else
    rtl_setrelopi(s, RELOP_NE, s0, s0, 0);
    rtl_set_CF(s, s0);
#endif
  }
}

def_EHelper(bsf) {
  rtl_decode_binary(s, false, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_setrelopi(s, RELOP_EQ, &cpu.cc_dest, dsrc1, 0);
    rtl_set_lazycc(s, NULL, NULL, NULL, LAZYCC_BS, 0);
#else
    rtl_setrelopi(s, RELOP_EQ, s0, dsrc1, 0);
    rtl_set_ZF(s, s0);
#endif
  }
  *ddest = bit_scan(*dsrc1, false);
  rtl_wb_r(s, ddest);
}

def_EHelper(bsr) {
  rtl_decode_binary(s, false, true);
  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->isa.flag_def != 0);
  if (need_update_eflags) {
#ifdef CONFIG_x86_CC_LAZY
    rtl_setrelopi(s, RELOP_EQ, &cpu.cc_dest, dsrc1, 0);
    rtl_set_lazycc(s, NULL, NULL, NULL, LAZYCC_BS, 0);
#else
    rtl_setrelopi(s, RELOP_EQ, s0, dsrc1, 0);
    rtl_set_ZF(s, s0);
#endif
  }
  *ddest = bit_scan(*dsrc1, true);
  rtl_wb_r(s, ddest);
}

def_EHelper(bts) {
  rtl_decode_binary(s, true, true);
  if (id_dest->type == OP_TYPE_MEM) { assert(0); }

  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);

  int need_update_eflags = MUXDEF(CONFIG_x86_CC_NONE, true, s->isa.flag_def != 0);
  if (need_update_eflags) {
    rtl_and(s, s1, s0, ddest);
#ifdef CONFIG_x86_CC_LAZY
    rtl_set_lazycc(s, s1, NULL, NULL, LAZYCC_BT, s->isa.width);
#else
    rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
    rtl_set_CF(s, s1);
#endif
  }

  rtl_or(s, ddest, ddest, s0);
  rtl_wb_r(s, ddest);
}

#if 0
static inline def_EHelper(btc) {
  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s1, s0, ddest);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);
  rtl_xor(s, ddest, ddest, s0);
  operand_write(s, id_dest, ddest);
}

static inline def_EHelper(btr) {
  rtl_li(s, s0, 1);
  rtl_sll(s, s0, s0, dsrc1);
  rtl_and(s, s1, s0, ddest);
  rtl_setrelopi(s, RELOP_NE, s1, s1, 0);
  rtl_set_CF(s, s1);
  rtl_not(s, s0, s0);
  rtl_and(s, ddest, ddest, s0);
  operand_write(s, id_dest, ddest);
}
#endif

def_EHelper(bswap) {
  // src[7:0]
  rtl_slli(s, s1, ddest, 24);

  // src[31:24]
  rtl_srli(s, s0, ddest, 24);
  rtl_or(s, s1, s1, s0);

  // src[15:8]
  rtl_slli(s, s0, ddest, 8);
  rtl_andi(s, s0, s0, 0xff0000);
  rtl_or(s, s1, s1, s0);

  // src[23:16]
  rtl_srli(s, s0, ddest, 8);
  rtl_andi(s, s0, s0, 0xff00);
  rtl_or(s, ddest, s1, s0);

  rtl_wb_r(s, ddest);
}
