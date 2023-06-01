#ifndef __SET_H__
#define __SET_H__

#include "common.h"

#define SET_SIZE (32 * 1024)

typedef struct {
    uint64_t table[SET_SIZE];
} set_t;

bool set_has(set_t *, uint64_t);
bool set_add(set_t *, uint64_t);
void set_reset(set_t *);

#endif
