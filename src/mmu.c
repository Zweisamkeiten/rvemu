/**
 * @file mmu.c
 * @brief
 */

#include "rvemu.h"

/**
 * @brief load program header
 *
 * @param phdr
 * @param ehdr
 * @param i
 * @param file
 */
static void load_phdr(elf64_phdr_t *phdr, elf64_ehdr_t *ehdr, size_t i,
                      FILE *file) {
  int ret = fseek(file, ehdr->e_phoff + i * ehdr->e_phentsize, SEEK_SET);
  Assert(ret == 0, "%s, seek file failed", strerror(errno));

  size_t ret_r = fread((void *)phdr, 1, sizeof(elf64_phdr_t), file);
  Assert(ret_r == sizeof(elf64_phdr_t), "file too small");
}

/**
 * @brief ELF program segment flags to mmap prot
 *
 * @param flags
 * @return
 */
static int flags_to_mmap_prot(uint32_t flags) {
  return (flags & PF_R ? PROT_READ : 0) | (flags & PF_W ? PROT_WRITE : 0) |
         (flags & PF_X ? PROT_EXEC : 0);
}

static void mmu_load_segment(mmu_t *mmu, elf64_phdr_t *phdr, int fd) {
  int page_size = getpagesize();

  uint64_t offset = phdr->p_offset;
  uint64_t vaddr = GUEST_TO_HOST(phdr->p_vaddr);
  uint64_t aligned_vaddr = ROUNDDOWN(vaddr, page_size);
  uint64_t filesz = phdr->p_filesz + (vaddr - aligned_vaddr);
  uint64_t memsz = phdr->p_memsz + (vaddr - aligned_vaddr);

  int prot = flags_to_mmap_prot(phdr->p_flags);

  uint64_t addr =
      (uint64_t)mmap((void *)aligned_vaddr, filesz, prot,
                     MAP_PRIVATE | MAP_FIXED, fd, ROUNDDOWN(offset, page_size));
  Assert(addr == aligned_vaddr, "aligned_vaddr is not equal to addr");

  uint64_t remaining_bss =
      ROUNDUP(memsz, page_size) - ROUNDUP(filesz, page_size);
  if (remaining_bss > 0) {
    uint64_t addr = (uint64_t)mmap(
        (void *)(aligned_vaddr + ROUNDUP(filesz, page_size)), remaining_bss,
        prot, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

    Assert(addr == aligned_vaddr + ROUNDUP(filesz, page_size),
           "aligned_vaddr is not equal to addr");
  }

  // [             | host_alloc        ]
  // [     ELF     | malloc    |       ]
  // [             | base      | alloc ]
  mmu->host_alloc =
      MAX(mmu->host_alloc, (aligned_vaddr + ROUNDUP(memsz, page_size)));

  mmu->base = mmu->alloc = HOST_TO_GUEST(mmu->host_alloc);
}

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
  Assert(*(uint32_t *)(ehdr->e_ident) == *(uint32_t *)ELFMAG,
         "Magic number check failed");

  Assert(ehdr->e_machine == EM_RISCV || ehdr->e_ident[EI_CLASS] == ELFCLASS64,
         "only riscv64 elf file is supported");

  mmu->entry = (uint64_t)ehdr->e_entry;

  for (size_t i = 0; i < ehdr->e_phnum; i++) {
    elf64_phdr_t phdr;
    load_phdr(&phdr, ehdr, i, file);

    if (phdr.p_type == PT_LOAD) {
      mmu_load_segment(mmu, &phdr, fd);
    }
  }
}

/**
 * @brief mmu_alloc 申请释放内存
 *
 * @param mmu
 * @param size 有符号数, 为负释放内存
 * @return
 */
uint64_t mmu_alloc(mmu_t *mmu, int64_t size) {
  int page_size = getpagesize();
  uint64_t base = mmu->alloc;
  Assert(base >= mmu->base, "alloc cant be smaller than mmu base");

  mmu->alloc += size;
  Assert(mmu->alloc >= mmu->base, "alloc cant be smaller than mmu base");

  if (size > 0 && mmu->alloc > HOST_TO_GUEST(mmu->host_alloc)) {
    if (mmap((void *)mmu->host_alloc, ROUNDUP(size, page_size),
             PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1,
             0) == MAP_FAILED) {
      panic("mmap failed");
    }

    mmu->host_alloc += ROUNDUP(size, page_size);
  } else if (size < 0 &&
             ROUNDUP(mmu->alloc, page_size) < HOST_TO_GUEST(mmu->host_alloc)) {
    uint64_t len =
        HOST_TO_GUEST(mmu->host_alloc) - ROUNDUP(mmu->alloc, page_size);
    if (munmap((void *)mmu->host_alloc, len) == -1)
      fatal(strerror(errno));
    mmu->host_alloc -= len;
  }

  return base;
}
