#ifndef __STACK_H__
#define __STACK_H__

#include "common.h"

#define STACK_CAP 256
typedef struct {
  int64_t top;
  uint64_t elems[STACK_CAP];
} stack_t;

extern void stack_push(stack_t *stack, uint64_t elem);
extern bool stack_pop(stack_t *stack, uint64_t *elem);
extern void stack_reset(stack_t *stack);
void stack_print(stack_t *stack);

#endif
