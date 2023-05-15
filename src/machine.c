/**
 * @file machine.c
 * @brief
 */

#include "rvemu.h"
#include "mmu.h"

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
