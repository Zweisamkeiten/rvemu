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
    m->state.exit_reason = none;
    exec_block_interp(&m->state);
    Assert(m->state.exit_reason != none,
           "exec block interp exit reason is None");

    if (m->state.exit_reason == indirect_branch ||
        m->state.exit_reason == direct_branch) {
      m->state.pc = m->state.reenter_pc;
      continue;
    }

    break;
  }

  m->state.pc = m->state.reenter_pc;
  Assert(m->state.exit_reason == ecall, "machine exit reason is not ecall");
  return ecall;
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
