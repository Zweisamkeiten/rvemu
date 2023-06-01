#include "rvemu.h"

#define sys_icache_invalidate(addr, size)                                      \
  __builtin___clear_cache((char *)(addr), (char *)(addr) + (size));

static uint64_t hash(uint64_t pc) { return pc % CACHE_ENTRY_SIZE; }

/**
 * @brief 创建完整 cache 组织结构
 *
 * @return
 */
cache_t *new_cache() {
  cache_t *cache = (cache_t *)calloc(1, sizeof(cache_t));
  cache->jitcode =
      (uint8_t *)mmap(NULL, CACHE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  return cache;
}

#define MAX_SEARCH_COUNT 32
#define CACHE_HOT_COUNT 100000

#define CACHE_IS_HOT (cache->table[index].hot >= CACHE_HOT_COUNT)

/**
 * @brief 查找PC是否在cache缓存中, hash直到table空项
 *
 * @param cache 
 * @param pc 
 * @return 
 */
uint8_t *cache_lookup(cache_t *cache, uint64_t pc) {
  Assert(pc != 0, "pc == 0 wrong");

  uint64_t index = hash(pc);

  while (cache->table[index].pc != 0) {
    if (cache->table[index].pc == pc) {
      if (CACHE_IS_HOT)
        return cache->jitcode + cache->table[index].offset;
      break;
    }

    index++;
    index = hash(index);
  }

  return NULL;
}

static inline uint64_t align_to(uint64_t val, uint64_t align) {
  if (align == 0)
    return val;
  return (val + align - 1) & ~(align - 1);
}

uint8_t *cache_add(cache_t *cache, uint64_t pc, uint8_t *code, size_t sz,
                   uint64_t align) {
  cache->offset = align_to(cache->offset, align);
  assert(cache->offset + sz <= CACHE_SIZE);

  uint64_t index = hash(pc);
  uint64_t search_count = 0;
  while (cache->table[index].pc != 0) {
    if (cache->table[index].pc == pc) {
      break;
    }

    index++;
    index = hash(index);

    assert(++search_count <= MAX_SEARCH_COUNT);
  }

  memcpy(cache->jitcode + cache->offset, code, sz);
  cache->table[index].pc = pc;
  cache->table[index].offset = cache->offset;
  cache->offset += sz;
  sys_icache_invalidate(cache->jitcode + cache->table[index].offset, sz);
  return cache->jitcode + cache->table[index].offset;
}

bool cache_hot(cache_t *cache, uint64_t pc) {
  uint64_t index = hash(pc);
  uint64_t search_count = 0;
  while (cache->table[index].pc != 0) {
    if (cache->table[index].pc == pc) {
      cache->table[index].hot = MIN(++cache->table[index].hot, CACHE_HOT_COUNT);
      return CACHE_IS_HOT;
    }

    index++;
    index = hash(index);

    Assert(++search_count <= MAX_SEARCH_COUNT,
           "cache search count > MAX_SEARCH_COUNT");
  }

  cache->table[index].pc = pc;
  cache->table[index].hot = 1;
  return false;
}
