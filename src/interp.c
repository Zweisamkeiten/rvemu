/**
 * @file interp.c
 * @brief interptor 解释执行
 */

#include "decode.h"
#include "rvemu.h"

typedef void(func_t)(state_t *, inst_t *);

static func_t *funcs[] = {};

/**
 * @brief exec until a branch or syscall come
 *
 * @param state
 */
void exec_block_interp(state_t *state) {
  static inst_t inst = {0};

  while (true) {
    uint32_t data = *(uint32_t *)GUEST_TO_HOST(state->pc);
    inst_decode(&inst, data);

    funcs[inst.type](state, &inst);
    state->gp_regs[zero] = 0;

    // syscall
    if (inst.cont)
      break;

    state->pc += inst.rvc ? 2 : 4;
  }
}
