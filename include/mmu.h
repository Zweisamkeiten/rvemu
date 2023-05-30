#ifndef __MMU_H__
#define __MMU_H__

#include "rvemu.h"

void mmu_load_elf(mmu_t *mmu, int fd);
uint64_t mmu_alloc(mmu_t *mmu, int64_t size);

inline void mmu_write(uint64_t addr, uint8_t *data, size_t len) {
    memcpy((void *)GUEST_TO_HOST(addr), (void *)data, len);
}

#endif
