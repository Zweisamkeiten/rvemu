#ifndef __RVEMU_H__
#define __RVEMU_H__

#include "common.h"

enum inst_type_t {
  inst_addi,
  num_inst,
};

typedef struct {
  int8_t rd;
  int8_t rs1;
  int8_t rs2;
  int32_t imm;
  enum inst_type_t type;
  bool rvc;
  bool cont;
} inst_t;

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
enum exit_reason_t {
  none,
  direct_branch,
  indirect_branch,
  ecall,
};

typedef struct {
  enum exit_reason_t exit_reason;
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
#include "reg.h"

enum exit_reason_t machine_step(machine_t *m);
void machine_load_program(machine_t *m, const char *prog);

#endif
