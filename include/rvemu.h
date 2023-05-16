#ifndef __RVEMU_H__
#define __RVEMU_H__

#include "common.h"

/*
 * mmu.c
 */
typedef struct {
  uint64_t entry;
  uint64_t host_alloc;
  uint64_t alloc;
  uint64_t base;
} mmu_t;

/*
 * state.c
 */
typedef struct {
  uint64_t gp_regs[32];
  uint64_t pc;
} state_t;

/*
 * machine.c
 */
typedef struct {
  state_t state;
  mmu_t mmu;
} machine_t;

#include "elfdef.h"

void machine_load_program(machine_t *m, const char *prog);

#endif
