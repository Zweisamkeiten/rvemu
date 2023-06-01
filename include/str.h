#ifndef __STR_H__
#define __STR_H__

#include "common.h"

#define STR_MAX_PREALLOC (1024 * 1024)
#define STRHDR(s) ((strhdr_t *)((s) - (sizeof(strhdr_t))))

#define DECLEAR_STATIC_STR(name)                                               \
  static str_t name = NULL;                                                    \
  if (name)                                                                    \
    str_clear(name);                                                           \
  else                                                                         \
    name = str_new();

typedef char *str_t;

#define STRHDR(s) ((strhdr_t *)((s) - (sizeof(strhdr_t))))
typedef struct {
  uint64_t len;
  uint64_t alloc;
  char buf[];
} strhdr_t;

inline str_t str_new() {
  strhdr_t *h = (strhdr_t *)calloc(1, sizeof(strhdr_t));
  return h->buf;
}

inline size_t str_len(const str_t str) { return STRHDR(str)->len; }

void str_clear(str_t);

str_t str_append(str_t, const char *);

#endif
