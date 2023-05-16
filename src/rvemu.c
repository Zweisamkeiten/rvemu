#include "rvemu.h"

int main(int argc, char *argv[]) {

  Assert(argc > 1, "Usage: ./rvemu ./playground/a.out");

  machine_t machine;
  machine_load_program(&machine, argv[1]);

  printf("entry: %llx\n", GUEST_TO_HOST(machine.mmu.entry));
  printf("host_alloc: %lx\n", machine.mmu.host_alloc);

  return EXIT_SUCCESS;
}
