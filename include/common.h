#ifndef __COMMON_H__
#define __COMMON_H__

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

#define fatalf(fmt, ...)                                                       \
  (fprintf(stderr, "fatal: %s: %d " fmt "\n", __FILE__, __LINE__,              \
           __VA_ARGS__),                                                       \
   exit(1))
#define fatal(msg) fatalf("%s", msg)

#define Assert(cond, format, ...)                                              \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "fatal: %s: %d " ANSI_FMT(format, ANSI_FG_RED) "\n",     \
              __FILE__, __LINE__, ##__VA_ARGS__),                              \
          assert(cond);                                                        \
    }                                                                          \
  } while (0)

#endif
