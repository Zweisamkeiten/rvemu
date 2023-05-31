/**
 * @file interp.c
 * @brief interptor 解释执行
 */

#include "decode.h"
#include "interp_util.h"
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

static void func_empty(state_t *state, inst_t *inst) { panic("unimplement"); }

#define FUNC_LOAD(inst, typ)                                                   \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t addr = state->gp_regs[inst->rs1] + (int64_t)inst->imm;            \
    state->gp_regs[inst->rd] = *(typ *)GUEST_TO_HOST(addr);                    \
  }

FUNC_LOAD(lb, int8_t);
FUNC_LOAD(lh, int16_t);
FUNC_LOAD(lw, int32_t);
FUNC_LOAD(ld, int64_t);
FUNC_LOAD(lbu, uint8_t);
FUNC_LOAD(lhu, uint16_t);
FUNC_LOAD(lwu, uint32_t);

#define FUNC_ALUI(inst, expr)                                                  \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t rs1 = state->gp_regs[inst->rs1];                                  \
    int64_t imm = (int64_t)inst->imm;                                          \
    state->gp_regs[inst->rd] = (expr);                                         \
  }

FUNC_ALUI(addi, rs1 + imm);
FUNC_ALUI(slli, rs1 << (imm & 0x3f));
FUNC_ALUI(slti, (int64_t)rs1 << (int64_t)imm);
FUNC_ALUI(sltiu, (uint64_t)rs1 << (uint64_t)imm);
FUNC_ALUI(xori, rs1 &imm);
FUNC_ALUI(srli, (uint64_t)rs1 >> (uint64_t)(imm & 0x3f));
FUNC_ALUI(srai, (int64_t)rs1 >> (int64_t)(imm & 0x3f));
FUNC_ALUI(ori, rs1 | (uint64_t)imm);
FUNC_ALUI(andi, rs1 &(uint64_t)imm);
FUNC_ALUI(addiw, (int64_t)(int32_t)(rs1 + imm));
FUNC_ALUI(slliw, (int64_t)(int32_t)(rs1 << (imm & 0x1f)));
FUNC_ALUI(srliw, (int64_t)(int32_t)((uint32_t)rs1 >> (imm & 0x1f)));
FUNC_ALUI(sraiw, (int64_t)((int32_t)rs1 >> (imm & 0x1f)));

static void func_auipc(state_t *state, inst_t *inst) {
  uint64_t val = state->pc + (int64_t)inst->imm;
  state->gp_regs[inst->rd] = val;
}

#define FUNC_STORE(inst, typ)                                                  \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t rs1 = state->gp_regs[inst->rs1];                                  \
    uint64_t rs2 = state->gp_regs[inst->rs2];                                  \
    IFDEF(CONFIG_DEBUG, printf("%llx\n", GUEST_TO_HOST(rs1 + inst->imm)));     \
    *(typ *)GUEST_TO_HOST(rs1 + inst->imm) = (typ)rs2;                         \
  }

FUNC_STORE(sb, uint8_t);
FUNC_STORE(sh, uint16_t);
FUNC_STORE(sw, uint32_t);
FUNC_STORE(sd, uint64_t);

#define FUNC_ALU(inst, expr)                                                   \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t rs1 = state->gp_regs[inst->rs1];                                  \
    uint64_t rs2 = state->gp_regs[inst->rs2];                                  \
    state->gp_regs[inst->rd] = (expr);                                         \
  }

FUNC_ALU(add, rs1 + rs2);
FUNC_ALU(sll, rs1 << (rs2 & 0x3f));
FUNC_ALU(slt, (int64_t)rs1 < (int64_t)rs2);
FUNC_ALU(sltu, (uint64_t)rs1 < (uint64_t)rs2);
FUNC_ALU(xor, rs1 ^ rs2);
FUNC_ALU(srl, rs1 >> (rs2 & 0x3f));
FUNC_ALU(or, rs1 | rs2);
FUNC_ALU(and, rs1 &rs2);

FUNC_ALU(mul, rs1 *rs2);
FUNC_ALU(mulh, mulh_helper(rs1, rs2));
FUNC_ALU(mulhsu, mulhsu_helper(rs1, rs2));
FUNC_ALU(mulhu, mulhu_helper(rs1, rs2));

FUNC_ALU(sub, rs1 - rs2);
FUNC_ALU(sra, (int64_t)rs1 >> (rs2 & 0x3f));
FUNC_ALU(remu, rs2 == 0 ? rs1 : rs1 % rs2);
FUNC_ALU(addw, (int64_t)(int32_t)(rs1 + rs2));
FUNC_ALU(sllw, (int64_t)(int32_t)(rs1 << (rs2 & 0x1f)));
FUNC_ALU(srlw, (int64_t)(int32_t)((uint32_t)rs1 >> (rs2 & 0x1f)));
FUNC_ALU(mulw, (int64_t)(int32_t)(rs1 *rs2));
FUNC_ALU(divw, rs2 == 0
                   ? UINT64_MAX
                   : (int32_t)((int64_t)(int32_t)rs1 / (int64_t)(int32_t)rs2));
FUNC_ALU(divuw,
         rs2 == 0 ? UINT64_MAX : (int32_t)((uint32_t)rs1 / (uint32_t)rs2));
FUNC_ALU(remw, rs2 == 0 ? (int64_t)(int32_t)rs1
                        : (int64_t)(int32_t)((int64_t)(int32_t)rs1 %
                                             (int64_t)(int32_t)rs2));
FUNC_ALU(remuw, rs2 == 0 ? (int64_t)(int32_t)(uint32_t)rs1
                         : (int64_t)(int32_t)((uint32_t)rs1 % (uint32_t)rs2));
FUNC_ALU(subw, (int64_t)(int32_t)(rs1 - rs2));
FUNC_ALU(sraw, (int64_t)(int32_t)((int32_t)rs1 >> (rs2 & 0x1f)));

#define FUNC_BR(inst, expr)                                                    \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t rs1 = state->gp_regs[inst->rs1];                                  \
    uint64_t rs2 = state->gp_regs[inst->rs2];                                  \
    uint64_t target_addr = state->pc + (int64_t)inst->imm;                     \
    if (expr) {                                                                \
      state->reenter_pc = state->pc = target_addr;                             \
      state->exit_reason = direct_branch;                                      \
      inst->cont = true;                                                       \
    }                                                                          \
  }

FUNC_BR(beq, (uint64_t)rs1 == (uint64_t)rs2);
FUNC_BR(bne, (uint64_t)rs1 != (uint64_t)rs2);
FUNC_BR(blt, (int64_t)rs1 < (int64_t)rs2);
FUNC_BR(bge, (int64_t)rs1 >= (int64_t)rs2);
FUNC_BR(bltu, (uint64_t)rs1 < (uint64_t)rs2);
FUNC_BR(bgeu, (uint64_t)rs1 >= (uint64_t)rs2);

static void func_jalr(state_t *state, inst_t *inst) {
  uint64_t rs1 = state->gp_regs[inst->rs1];
  state->gp_regs[inst->rd] = state->pc + (inst->rvc ? 2 : 4);
  state->exit_reason = indirect_branch;
  state->reenter_pc = (rs1 + (int64_t)inst->imm) & ~(uint64_t)1;
}

static void func_jal(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = state->pc + (inst->rvc ? 2 : 4);
  state->reenter_pc = state->pc = state->pc + (int64_t)inst->imm;
  state->exit_reason = direct_branch;
}

static void func_ecall(state_t *state, inst_t *inst) {
  state->exit_reason = ecall;
  state->reenter_pc = state->pc + 4;
}

static void func_lui(state_t *state, inst_t *inst) {
  state->gp_regs[inst->rd] = (int64_t)inst->imm;
}

static void func_div(state_t *state, inst_t *inst) {
  uint64_t rs1 = state->gp_regs[inst->rs1];
  uint64_t rs2 = state->gp_regs[inst->rs2];
  uint64_t rd = 0;
  if (rs2 == 0) {
    rd = UINT64_MAX;
  } else if (rs1 == INT64_MIN && rs2 == UINT64_MAX) {
    rd = INT64_MIN;
  } else {
    rd = (int64_t)rs1 / (int64_t)rs2;
  }
  state->gp_regs[inst->rd] = rd;
}

static void func_divu(state_t *state, inst_t *inst) {
  uint64_t rs1 = state->gp_regs[inst->rs1];
  uint64_t rs2 = state->gp_regs[inst->rs2];
  uint64_t rd = 0;
  if (rs2 == 0) {
    rd = UINT64_MAX;
  } else {
    rd = rs1 / rs2;
  }
  state->gp_regs[inst->rd] = rd;
}

#define FUNC_FSTORE(inst, typ)                                                 \
  static void func_##inst(state_t *state, inst_t *inst) {                      \
    uint64_t rs1 = state->gp_regs[inst->rs1];                                  \
    uint64_t rs2 = state->fp_regs[inst->rs2].v;                                \
    *(typ *)GUEST_TO_HOST(rs1 + inst->imm) = (typ)rs2;                         \
  }

FUNC_FSTORE(fsw, uint32_t);
FUNC_FSTORE(fsd, uint32_t);

static void func_flw(state_t *state, inst_t *inst) {
  uint64_t addr = state->gp_regs[inst->rs1] + (int64_t)inst->imm;
  state->fp_regs[inst->rd].v =
      *(uint32_t *)GUEST_TO_HOST(addr) | ((uint64_t)-1 << 32);
}
static void func_fld(state_t *state, inst_t *inst) {
  uint64_t addr = state->gp_regs[inst->rs1] + (int64_t)inst->imm;
  state->fp_regs[inst->rd].v = *(uint64_t *)GUEST_TO_HOST(addr);
}

typedef void(func_t)(state_t *, inst_t *);

static func_t *funcs[] = {
    func_lb,    func_lh,    func_lw,     func_ld,    func_lbu,   func_lhu,
    func_lwu,

    func_empty, func_empty,

    func_addi,  func_slli,  func_slti,   func_sltiu, func_xori,  func_srli,
    func_srai,  func_ori,   func_andi,   func_auipc, func_addiw, func_slliw,
    func_srliw, func_sraiw,

    func_sb,    func_sh,    func_sw,     func_sd,

    func_add,   func_sll,   func_slt,    func_sltu,  func_xor,   func_srl,
    func_or,    func_and,

    func_mul,   func_mulh,  func_mulhsu, func_mulhu, func_div,   func_divu,
    func_empty, func_remu,

    func_sub,   func_sra,   func_lui,

    func_addw,  func_sllw,  func_srlw,   func_mulw,  func_divw,  func_divuw,
    func_remw,  func_remuw, func_subw,   func_sraw,

    func_beq,   func_bne,   func_blt,    func_bge,   func_bltu,  func_bgeu,

    func_jalr,  func_jal,   func_ecall,  func_empty,

    func_empty, func_empty, func_empty,  func_empty, func_empty, func_empty,

    func_flw,   func_fsw,

    func_empty, func_empty, func_empty,  func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty,  func_empty,

    func_empty, func_empty, func_empty,  func_empty, func_empty,

    func_empty, func_empty,

    func_fld,   func_fsd,

    func_empty, func_empty, func_empty,  func_empty,

    func_empty, func_empty, func_empty,  func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty,  func_empty,

    func_empty, func_empty, func_empty,  func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty,  func_empty,
};

/**
 * @brief exec until a branch or syscall come
 *
 * @param state
 */
void exec_block_interp(state_t *state) {
  static inst_t inst = {0};

  while (true) {
    IFDEF(CONFIG_DEBUG, printf("pc: %lx\n", state->pc));
    uint32_t data = *(uint32_t *)GUEST_TO_HOST(state->pc);
    inst_decode(&inst, data);
    IFDEF(CONFIG_DEBUG, printf("data: %#010x\n", data));
    IFDEF(CONFIG_DEBUG, fprintf(stderr, "inst: %s\n", inst_name[inst.type]));

    IFDEF(CONFIG_DEBUG, printf("%s\n", inst_name[inst.type]));
    funcs[inst.type](state, &inst);
    state->gp_regs[zero] = 0;

    // syscall
    if (inst.cont)
      break;

    state->pc += inst.rvc ? 2 : 4;
  }
}
