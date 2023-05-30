/**
 * @file interp.c
 * @brief interptor 解释执行
 */

#include "decode.h"
#include "rvemu.h"

const char *inst_name[] = {
    "inst_lb",        "inst_lh",        "inst_lw",        "inst_ld",
    "inst_lbu",       "inst_lhu",       "inst_lwu",

    "inst_fence",     "inst_fence_i",

    "inst_addi",      "inst_slli",      "inst_slti",      "inst_sltiu",
    "inst_xori",      "inst_srli",      "inst_srai",      "inst_ori",
    "inst_andi",      "inst_auipc",     "inst_addiw",     "inst_slliw",
    "inst_srliw",     "inst_sraiw",

    "inst_sb",        "inst_sh",        "inst_sw",        "inst_sd",

    "inst_add",       "inst_sll",       "inst_slt",       "inst_sltu",
    "inst_xor",       "inst_srl",       "inst_or",        "inst_and",

    "inst_mul",       "inst_mulh",      "inst_mulhsu",    "inst_mulhu",
    "inst_div",       "inst_divu",      "inst_rem",       "inst_remu",

    "inst_sub",       "inst_sra",       "inst_lui",

    "inst_addw",      "inst_sllw",      "inst_srlw",      "inst_mulw",
    "inst_divw",      "inst_divuw",     "inst_remw",      "inst_remuw",
    "inst_subw",      "inst_sraw",

    "inst_beq",       "inst_bne",       "inst_blt",       "inst_bge",
    "inst_bltu",      "inst_bgeu",

    "inst_jalr",      "inst_jal",       "inst_ecall",     "inst_ebreak",

    "inst_csrrc",     "inst_csrrci",    "inst_csrrs",     "inst_csrrsi",
    "inst_csrrw",     "inst_csrrwi",

    "inst_flw",       "inst_fsw",

    "inst_fmadd_s",   "inst_fmsub_s",   "inst_fnmsub_s",  "inst_fnmadd_s",
    "inst_fadd_s",    "inst_fsub_s",    "inst_fmul_s",    "inst_fdiv_s",
    "inst_fsqrt_s",

    "inst_fsgnj_s",   "inst_fsgnjn_s",  "inst_fsgnjx_s",

    "inst_fmin_s",    "inst_fmax_s",

    "inst_fcvt_w_s",  "inst_fcvt_wu_s", "inst_fmv_x_w",

    "inst_feq_s",     "inst_flt_s",     "inst_fle_s",     "inst_fclass_s",

    "inst_fcvt_s_w",  "inst_fcvt_s_wu", "inst_fmv_w_x",   "inst_fcvt_l_s",
    "inst_fcvt_lu_s",

    "inst_fcvt_s_l",  "inst_fcvt_s_lu",

    "inst_fld",       "inst_fsd",

    "inst_fmadd_d",   "inst_fmsub_d",   "inst_fnmsub_d",  "inst_fnmadd_d",

    "inst_fadd_d",    "inst_fsub_d",    "inst_fmul_d",    "inst_fdiv_d",
    "inst_fsqrt_d",

    "inst_fsgnj_d",   "inst_fsgnjn_d",  "inst_fsgnjx_d",

    "inst_fmin_d",    "inst_fmax_d",

    "inst_fcvt_s_d",  "inst_fcvt_d_s",

    "inst_feq_d",     "inst_flt_d",     "inst_fle_d",     "inst_fclass_d",

    "inst_fcvt_w_d",  "inst_fcvt_wu_d", "inst_fcvt_d_w",  "inst_fcvt_d_wu",

    "inst_fcvt_l_d",  "inst_fcvt_lu_d",

    "inst_fmv_x_d",   "inst_fcvt_d_l",  "inst_fcvt_d_lu", "inst_fmv_d_x",
};

static void func_empty(state_t *state, inst_t *inst) {}

typedef void(func_t)(state_t *, inst_t *);

static func_t *funcs[] = {
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty};

/**
 * @brief exec until a branch or syscall come
 *
 * @param state
 */
void exec_block_interp(state_t *state) {
  static inst_t inst = {0};

  while (true) {
    uint32_t data = *(uint32_t *)GUEST_TO_HOST(state->pc);
    // printf("%lx\n", state->pc);
    inst_decode(&inst, data);
    IFDEF(CONFIG_DEBUG, printf("data: %#010x\n", data));
    IFDEF(CONFIG_DEBUG, fprintf(stderr, "inst: %s\n", inst_name[inst.type]));

    funcs[inst.type](state, &inst);
    state->gp_regs[zero] = 0;

    // syscall
    if (inst.cont)
      break;

    state->pc += inst.rvc ? 2 : 4;
  }
}
