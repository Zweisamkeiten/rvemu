#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CACHE_ENTRY_SIZE (64 * 1024)
#define CACHE_SIZE (64 * 1024 * 1024)

typedef struct {
  uint64_t pc;
  uint64_t hot;
  uint64_t offset;
} cache_item_t;

typedef struct {
  uint8_t *jitcode;
  uint64_t offset;
  cache_item_t table[CACHE_ENTRY_SIZE];
} cache_t;

extern cache_t *new_cache();
extern uint8_t *cache_lookup(cache_t *cache, uint64_t pc);
extern uint8_t *cache_add(cache_t *, uint64_t, uint8_t *, size_t, uint64_t);
extern bool cache_hot(cache_t *, uint64_t);

#endif
