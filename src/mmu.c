#include "rvemu.h"

/**
 * @brief mmu load elf
 *
 * @param mmu
 * @param fd
 */
void mmu_load_elf(mmu_t *mmu, int fd) {
  uint8_t buf[sizeof(elf64_ehdr_t)];

  FILE *file = fdopen(fd, "rb");
  Assert(file != NULL, "File open failed");
  int fsize = fread(buf, 1, sizeof(elf64_ehdr_t), file);
  Assert(fsize == sizeof(elf64_ehdr_t), "File too small");

  elf64_ehdr_t *ehdr = (elf64_ehdr_t *)buf;
  Assert(*(uint32_t *)(ehdr->e_ident) == *(uint32_t *)ELFMAG, "Magic number check failed");

  Assert(ehdr->e_machine == EM_RISCV || ehdr->e_ident[EI_CLASS] == ELFCLASS64,
         "only riscv64 elf file is supported");

  mmu->entry = (uint64_t)ehdr->e_entry;
}
