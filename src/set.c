#include "set.h"
#include "rvemu.h"

static inline uint64_t hash(uint64_t elem) { return elem % SET_SIZE; }

bool set_has(set_t *set, uint64_t elem) {
  assert(elem != 0);

  uint64_t index = hash(elem);

  while (set->table[index] != 0) {
    if (set->table[index] == elem) {
      return true;
    }
  }

  return false;
}

#define MAX_SEARCH_COUNT 32

bool set_add(set_t *set, uint64_t elem) {
  assert(elem != 0);

  uint64_t index = hash(elem);
  uint64_t search_count = 0;
  while (set->table[index] != 0) {
    if (set->table[index] == elem)
      return false;

    index++;
    index = hash(index);

    assert(++search_count <= MAX_SEARCH_COUNT);
  }

  set->table[index] = elem;
  return true;
}

void set_reset(set_t *set) { memset(set, 0, sizeof(set_t)); }
