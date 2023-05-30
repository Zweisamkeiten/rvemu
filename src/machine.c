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
