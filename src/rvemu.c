#include "rvemu.h"

int main(int argc, char *argv[]) {

  Assert(argc > 1, "Usage: ./rvemu ./playground/a.out");

  machine_t machine;
  machine_load_program(&machine, argv[1]);

  printf("entry: %lx\n", machine.mmu.entry);

  return EXIT_SUCCESS;
}
