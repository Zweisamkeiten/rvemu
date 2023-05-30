#include "rvemu.h"

int main(int argc, char *argv[]) {

  Assert(argc > 1, "Usage: ./rvemu ./playground/a.out");

  machine_t machine;
  machine_load_program(&machine, argv[1]);
  machine_setup(&machine, argc, argv);

  while (true) {
    enum exit_reason_t reason = machine_step(&machine);
    Assert(reason == ecall, "exit reason is not ecall");

    uint64_t syscall = machine_get_gp_reg(&machine, a7);
    uint64_t ret = do_syscall(&machine, syscall);
    machine_set_gp_reg(&machine, a0, ret);
  }

  return EXIT_SUCCESS;
}
