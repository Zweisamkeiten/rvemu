/**
 * @file machine.c
 * @brief
 */

#include "interp.h"
#include "mmu.h"
#include "rvemu.h"

/**
 * @brief exec a program block every step
 *
 * @param m
 * @return
 */
enum exit_reason_t machine_step(machine_t *m) {
  while (true) {

    bool hot = true;

    uint8_t *code = cache_lookup(m->cache, m->state.pc);
    if (code == NULL) {
      hot = cache_hot(m->cache, m->state.pc);
      if (hot) {
        str_t source = machine_genblock(m);
        code = machine_compile(m, source);
      }
    }

    if (!hot) {
      code = (uint8_t *)exec_block_interp;
    }

    while (true) {
      m->state.exit_reason = none;
      ((exec_block_func_t)code)(&m->state);
      Assert(m->state.exit_reason != none,
             "exec block interp exit reason is None");

      if (m->state.exit_reason == indirect_branch ||
          m->state.exit_reason == direct_branch) {
        // 发现退出的基本块要到达的基本块在cache中, 直接进入 cache 块执行
        code = cache_lookup(m->cache, m->state.reenter_pc);
        if (code != NULL)
          continue;
      }

      if (m->state.exit_reason == interp) {
        m->state.pc = m->state.reenter_pc;
        code = (uint8_t *)exec_block_interp;
        continue;
      }

      break;
    }

    // ecall or indirect&direct branch block not in cache
    m->state.pc = m->state.reenter_pc;
    switch (m->state.exit_reason) {
    case direct_branch:
    case indirect_branch:
      // continue execution
      break;
    case ecall:
      return ecall;
    default:
      unreachable();
    }
  }
}

/**
 * @brief Load prog into machine
 *
 * @param m machine type
 * @param prog program to load
 */
void machine_load_program(machine_t *m, const char *prog) {
  int fd = open(prog, O_RDONLY);
  Assert(fd != -1, "%s", strerror(errno));

  mmu_load_elf(&m->mmu, fd);
  close(fd);

  m->state.pc = (uint64_t)m->mmu.entry;
}

void machine_setup(machine_t *m, int argc, char *argv[]) {
  size_t stack_size = 32 * 1024 * 1024;
  uint64_t stack = mmu_alloc(&m->mmu, stack_size);
  m->state.gp_regs[sp] = stack + stack_size;

  m->state.gp_regs[sp] -= 8; // auxv
  m->state.gp_regs[sp] -= 8; // envp
  m->state.gp_regs[sp] -= 8; // argv end

  uint64_t args = argc - 1;

  for (int i = args; i > 0; i--) {
    size_t len = strlen(argv[i]);

    uint64_t addr = mmu_alloc(&m->mmu, len + 1);
    mmu_write(addr, (uint8_t *)argv[i], len);
    m->state.gp_regs[sp] -= 8; // argv[i]
    mmu_write(m->state.gp_regs[sp], (uint8_t *)&addr, sizeof(uint64_t));
  }

  m->state.gp_regs[sp] -= 8; // argc
  mmu_write(m->state.gp_regs[sp], (uint8_t *)&args, sizeof(uint64_t));
}
