#include "../local-include/csr.h"
#include "../local-include/intr.h"
#include <rtl/fp.h>
#include <cpu/cpu.h>
#include <cpu/decode.h>

static uint32_t nemu_rm_cache = 0;
void fp_update_rm_cache(uint32_t rm) { nemu_rm_cache = rm; }

uint32_t isa_fp_translate_rm(uint32_t riscv64_rm) {
  if (riscv64_rm == 0b111) riscv64_rm = nemu_rm_cache;
  uint32_t table[] = {
    FPCALL_RM_RNE,
    FPCALL_RM_RTZ,
    FPCALL_RM_RDN,
    FPCALL_RM_RUP,
    FPCALL_RM_RMM
  };
  return table[riscv64_rm];
}

bool fp_enable() {
  return MUXDEF(CONFIG_MODE_USER, true, mstatus->fs != 0);
}

void fp_set_dirty() {
  // lazily update mstatus->sd when reading mstatus
  mstatus->fs = 3;
}

uint32_t isa_fp_get_rm(Decode *s) {
  uint32_t rm = s->isa.instr.fp.rm;
  if (likely(rm == 7)) { return nemu_rm_cache; }
  switch (rm) {
    case 0: return FPCALL_RM_RNE;
    case 1: return FPCALL_RM_RTZ;
    case 2: return FPCALL_RM_RDN;
    case 3: return FPCALL_RM_RUP;
    case 4: return FPCALL_RM_RMM;
    default: save_globals(s); longjmp_exception(EX_II);
  }
  return FPCALL_RM_RNE;
}

void isa_fp_set_ex(uint32_t ex) {
  uint32_t f = 0;
  if (ex & FPCALL_EX_NX) f |= 0x01;
  if (ex & FPCALL_EX_UF) f |= 0x02;
  if (ex & FPCALL_EX_OF) f |= 0x04;
  if (ex & FPCALL_EX_DZ) f |= 0x08;
  if (ex & FPCALL_EX_NV) f |= 0x10;
  fcsr->fflags.val = f;
  fp_set_dirty();
}
