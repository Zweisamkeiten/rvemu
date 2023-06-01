#include "decode.h"
#include "rvemu.h"
#include "set.h"
#include "stack.h"
#include "str.h"

typedef struct {
  bool gp_reg[num_gp_regs];
  bool fp_reg[num_fp_regs];
} tracer_t;

static void tracer_reset(tracer_t *t) { memset(t, 0, sizeof(tracer_t)); }

#define DEFINE_TRACE_USAGE(name)                                               \
  static void tracer_add_##name##_usage(tracer_t *t, ...) {                    \
    va_list ap;                                                                \
    va_start(ap, t);                                                           \
    int n;                                                                     \
    while ((n = va_arg(ap, int)) != -1)                                        \
      t->name[n] = true;                                                       \
    va_end(ap);                                                                \
  }

DEFINE_TRACE_USAGE(gp_reg);
DEFINE_TRACE_USAGE(fp_reg);

static str_t tracer_append_prologue(tracer_t *t, str_t s) {
  static char buf[128] = {0};

  for (int i = 1; i < num_gp_regs; i++) {
    if (!t->gp_reg[i])
      continue;
    sprintf(buf, "    uint64_t x%d = state->gp_regs[%d];\n", i, i);
    s = str_append(s, buf);
  }

  for (int i = 0; i < num_fp_regs; i++) {
    if (!t->fp_reg[i])
      continue;
    sprintf(buf, "    fp_reg_t f%d = state->fp_regs[%d];\n", i, i);
    s = str_append(s, buf);
  }

  return s;
}

static str_t tracer_append_epilogue(tracer_t *t, str_t s) {
  static char buf[128] = {0};

  for (int i = 1; i < num_gp_regs; i++) {
    if (!t->gp_reg[i])
      continue;
    sprintf(buf, "    state->gp_regs[%d] = x%d;\n", i, i);
    s = str_append(s, buf);
  }

  for (int i = 0; i < num_fp_regs; i++) {
    if (!t->fp_reg[i])
      continue;
    sprintf(buf, "    state->fp_regs[%d] = f%d;\n", i, i);
    s = str_append(s, buf);
  }

  return s;
}

static str_t func_empty(str_t s, inst_t *inst, tracer_t *tracer, stack_t *stack,
                        uint64_t pc) {
  panic("unimplement");
  return s;
}

static char funcbuf[128] = {0};
static char funcbuf2[128] = {0};

#define REG_SET_VAL(reg, val)                                                  \
  if ((reg) != 0) {                                                            \
    sprintf(funcbuf, "    x%d = %ldLL;\n", (reg), (val));                      \
    s = str_append(s, funcbuf);                                                \
  }

#define REG_SET_EXPR(reg, expr)                                                \
  if ((reg) != 0) {                                                            \
    sprintf(funcbuf, "    x%d = %s;\n", (reg), (expr));                        \
    s = str_append(s, funcbuf);                                                \
  }

#define REG_SET_EXPR(reg, expr)                                                \
  if ((reg) != 0) {                                                            \
    sprintf(funcbuf, "    x%d = %s;\n", (reg), (expr));                        \
    s = str_append(s, funcbuf);                                                \
  }

#define REG_GET(reg, name)                                                     \
  if ((reg) == zero) {                                                         \
    s = str_append(s, "    uint64_t " #name " = 0;\n");                        \
  } else {                                                                     \
    sprintf(funcbuf, "    uint64_t " #name " = x%d;\n", (reg));                \
    s = str_append(s, funcbuf);                                                \
  }

#define FREG_GET(reg, name, typ, field)                                        \
  sprintf(funcbuf, "    " #typ " " #name " = f%d." #field ";\n", (reg));       \
  s = str_append(s, funcbuf);

#define MEM_LOAD(addr, type, name)                                             \
  sprintf(funcbuf, "    %s " #name " = *(%s *)GUEST_TO_HOST(%s);\n", (type),   \
          (type), (addr));                                                     \
  s = str_append(s, funcbuf);

#define MEM_STORE(addr, type, data)                                            \
  sprintf(funcbuf, "    *(%s *)GUEST_TO_HOST(%s) = (%s)" #data ";\n", (type),  \
          (addr), (type));                                                     \
  s = str_append(s, funcbuf);

/*************************************************************************
 * LOAD INST
 *************************************************************************/

#define FUNC_LOAD(name, type)                                                  \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    sprintf(funcbuf2, "rs1 + (int64_t)%ldLL", (int64_t)inst->imm);             \
    MEM_LOAD(funcbuf2, type, rd);                                              \
    REG_SET_EXPR(inst->rd, "rd");                                              \
    tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rd, -1);                  \
    return s;                                                                  \
  }

FUNC_LOAD(lb, "int8_t");
FUNC_LOAD(lh, "int16_t");
FUNC_LOAD(lw, "int32_t");
FUNC_LOAD(ld, "int64_t");
FUNC_LOAD(lbu, "uint8_t");
FUNC_LOAD(lhu, "uint16_t");
FUNC_LOAD(lwu, "uint32_t");

/*************************************************************************
 * STORE INST
 *************************************************************************/
#define FUNC_STORE(name, type)                                                 \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    REG_GET(inst->rs2, rs2);                                                   \
    sprintf(funcbuf2, "rs1 + (int64_t)%ldLL", (int64_t)inst->imm);             \
    MEM_STORE(funcbuf2, type, rs2);                                            \
    tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rs2, -1);                 \
    return s;                                                                  \
  }

FUNC_STORE(sb, "uint8_t");
FUNC_STORE(sh, "uint16_t");
FUNC_STORE(sw, "uint32_t");
FUNC_STORE(sd, "uint64_t");

/*************************************************************************
 * ALUI INST
 *************************************************************************/

#define FUNC_ALUI(name, stmt)                                                  \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    stmt;                                                                      \
    REG_SET_EXPR(inst->rd, funcbuf2);                                          \
    tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rd, -1);                  \
    return s;                                                                  \
  }

FUNC_ALUI(addi,
          (sprintf(funcbuf2, "rs1 + (int64_t)%ldLL", (int64_t)inst->imm)));
FUNC_ALUI(slli, (sprintf(funcbuf2, "rs1 << %d", inst->imm & 0x3f)));
FUNC_ALUI(addiw, (sprintf(funcbuf2, "(int64_t)(int32_t)(rs1 + (int64_t)%ldLL)",
                          (int64_t)inst->imm)));

/*************************************************************************
 * ALU INST
 *************************************************************************/

#define FUNC_ALU(name, expr)                                                   \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    REG_GET(inst->rs2, rs2);                                                   \
    REG_SET_EXPR(inst->rd, expr);                                              \
    tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rs2, -1);                 \
    return s;                                                                  \
  }

FUNC_ALU(add, "rs1 + rs2");
FUNC_ALU(mulw, "(int64_t)(int32_t)(rs1 * rs2)");
FUNC_ALU(remw,
         "rs2 == 0 ? (int64_t)(int32_t)rs1 : "
         "(int64_t)(int32_t)((int64_t)(int32_t)rs1 % (int64_t)(int32_t)rs2)");

/*************************************************************************
 * BRANCH INST
 *************************************************************************/

#define FUNC_BRANCH(name, type, op)                                            \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    REG_GET(inst->rs2, rs2);                                                   \
    uint64_t target_addr = pc + (int64_t)inst->imm;                            \
    sprintf(funcbuf, "    if ((%s)rs1 %s (%s)rs2) {\n", type, op, type);       \
    s = str_append(s, funcbuf);                                                \
    sprintf(funcbuf, "      goto inst_%lx;\n", target_addr);                   \
    s = str_append(s, funcbuf);                                                \
    s = str_append(s, "   }\n");                                               \
    stack_push(stack, target_addr);                                            \
    tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rs2, -1);                 \
    return s;                                                                  \
  }

FUNC_BRANCH(bge, "int64_t", ">=");
FUNC_BRANCH(blt, "int64_t", "<");
FUNC_BRANCH(bne, "uint64_t", "!=");

static str_t func_lui(str_t s, inst_t *inst, tracer_t *tracer, stack_t *stack,
                      uint64_t pc) {
  tracer_add_gp_reg_usage(tracer, inst->rd, -1);
  REG_SET_VAL(inst->rd, (int64_t)inst->imm);
  return s;
}

static str_t func_jalr(str_t s, inst_t *inst, tracer_t *tracer, stack_t *stack,
                       uint64_t pc) {
  uint64_t return_addr = pc + (inst->rvc ? 2 : 4);
  REG_GET(inst->rs1, rs1);
  REG_SET_VAL(inst->rd, return_addr);

  s = str_append(s, "    state->exit_reason = indirect_branch;\n");
  sprintf(funcbuf,
          "    state->reenter_pc = (rs1 + (int64_t)%ldLL) & ~(uint64_t)1;\n",
          (int64_t)inst->imm);
  s = str_append(s, funcbuf);
  s = str_append(s, "    goto end;\n");
  s = str_append(s, "}\n");
  tracer_add_gp_reg_usage(tracer, inst->rs1, inst->rd, -1);
  return s;
}

static str_t func_jal(str_t s, inst_t *inst, tracer_t *tracer, stack_t *stack,
                      uint64_t pc) {
  uint64_t return_addr = pc + (inst->rvc ? 2 : 4);
  uint64_t target_addr = pc + (int64_t)inst->imm;

  REG_SET_VAL(inst->rd, return_addr);
  sprintf(funcbuf, "    goto inst_%lx;\n", target_addr);
  s = str_append(s, funcbuf);
  stack_push(stack, target_addr);
  s = str_append(s, "}\n");

  tracer_add_gp_reg_usage(tracer, inst->rd, -1);
  return s;
}

static str_t func_ecall(str_t s, inst_t *inst, tracer_t *tracer, stack_t *stack,
                        uint64_t pc) {
  s = str_append(s, "    state->exit_reason = ecall;\n");
  sprintf(funcbuf, "    state->reenter_pc = %luULL;\n", pc + 4);
  s = str_append(s, funcbuf);
  s = str_append(s, "    goto end;\n");
  s = str_append(s, "}\n");
  return s;
}

#define FUNC_FSTORE(name, typ)                                                 \
  static str_t func_##name(str_t s, inst_t *inst, tracer_t *tracer,            \
                           stack_t *stack, uint64_t pc) {                      \
    REG_GET(inst->rs1, rs1);                                                   \
    FREG_GET(inst->rs2, rs2, uint64_t, v);                                     \
    sprintf(funcbuf2, "rs1 + (int64_t)%ldLL", (int64_t)inst->imm);             \
    MEM_STORE(funcbuf2, typ, rs2);                                             \
    tracer_add_gp_reg_usage(tracer, inst->rs1, -1);                            \
    tracer_add_fp_reg_usage(tracer, inst->rs2, -1);                            \
    return s;                                                                  \
  }

FUNC_FSTORE(fsw, "uint32_t");
FUNC_FSTORE(fsd, "uint64_t");

typedef str_t(func_t)(str_t str, inst_t *inst, tracer_t *tracer, stack_t *stack,
                      uint64_t);

static func_t *funcs[] = {
    func_lb,    func_lh,    func_lw,    func_ld,    func_lbu,   func_lhu,
    func_lwu,

    func_empty, func_empty,

    func_addi,  func_slli,  func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty, func_empty, func_addiw, func_empty,
    func_empty, func_empty,

    func_sb,    func_sh,    func_sw,    func_sd,

    func_add,   func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty,

    func_empty, func_empty, func_lui,

    func_empty, func_empty, func_empty, func_mulw,  func_empty, func_empty,
    func_remw,  func_empty, func_empty, func_empty,

    func_empty, func_bne,   func_blt,   func_bge,   func_empty, func_empty,

    func_jalr,  func_jal,   func_ecall, func_empty,

    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,

    func_empty, func_fsw,

    func_empty, func_empty, func_empty, func_empty, func_empty, func_empty,
    func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_fsd,

    func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty,

    func_empty, func_empty,

    func_empty, func_empty, func_empty, func_empty,
};

#define CODEGEN_PROLOGUE                                                       \
  "#define OFFSET 0x088800000000ULL               \n"                          \
  "#define GUEST_TO_HOST(addr) (addr + OFFSET)    \n"                          \
  "enum exit_reason_t {                           \n"                          \
  "   none,                                       \n"                          \
  "   direct_branch,                              \n"                          \
  "   indirect_branch,                            \n"                          \
  "   interp,                                     \n"                          \
  "   ecall,                                      \n"                          \
  "};                                             \n"                          \
  "typedef union {                                \n"                          \
  "    uint64_t v;                                \n"                          \
  "    uint32_t w;                                \n"                          \
  "    double d;                                  \n"                          \
  "    float f;                                   \n"                          \
  "} fp_reg_t;                                    \n"                          \
  "typedef struct {                               \n"                          \
  "    enum exit_reason_t exit_reason;            \n"                          \
  "    uint64_t reenter_pc;                       \n"                          \
  "    uint64_t gp_regs[32];                      \n"                          \
  "    fp_reg_t fp_regs[32];                      \n"                          \
  "    uint64_t pc;                               \n"                          \
  "    uint32_t fcsr;                             \n"                          \
  "} state_t;                                     \n"                          \
  "void start(volatile state_t *restrict state) { \n"

#define CODEGEN_EPILOGUE "}"

str_t machine_genblock(machine_t *m) {
  DECLEAR_STATIC_STR(body);

  static stack_t stack = {0};
  stack_reset(&stack);

  static set_t set;
  set_reset(&set);

  static tracer_t tracer;
  tracer_reset(&tracer);

  stack_push(&stack, m->state.pc);

  uint64_t pc = -1;

  while (stack_pop(&stack, &pc)) {
    if (!set_add(&set, pc)) {
      continue;
    }

    static char buf[128] = {0};
    static inst_t inst = {0};

    sprintf(buf, "inst_%lx: {\n", pc);
    body = str_append(body, buf);

    uint32_t data = *(uint32_t *)GUEST_TO_HOST(pc);
    inst_decode(&inst, data);
    extern const char *inst_name[];
    IFDEF(CONFIG_DEBUG,
          printf("codegen: inst: %d, %s\n", inst.type, inst_name[inst.type]));
    body = funcs[inst.type](body, &inst, &tracer, &stack, pc);

    if (inst.cont)
      continue;

    pc += (inst.rvc ? 2 : 4);
    sprintf(buf, "    goto inst_%lx;\n", pc);
    body = str_append(body, buf);
    body = str_append(body, "}\n");
    stack_push(&stack, pc);
  }

  DECLEAR_STATIC_STR(source);
  source = str_append(source, "#include <stdint.h>\n");
  source = str_append(source, "#include <stdbool.h>\n");
  source = str_append(source, CODEGEN_PROLOGUE);
  source = tracer_append_prologue(&tracer, source);
  source = str_append(source, body);
  source = str_append(source, "end:;\n");
  source = tracer_append_epilogue(&tracer, source);
  source = str_append(source, CODEGEN_EPILOGUE);

  // printf("%s\n", source);

  return source;
}
