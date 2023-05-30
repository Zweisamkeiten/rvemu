#ifndef __INTERP_UTIL_H__
#define __INTERP_UTIL_H__

#include <stdbool.h>
#include <stdint.h>

inline uint64_t mulhu_helper(uint64_t a, uint64_t b) {
  uint64_t t;
  uint32_t y1, y2, y3;
  uint64_t a0 = (uint32_t)a, a1 = a >> 32;
  uint64_t b0 = (uint32_t)b, b1 = b >> 32;
  t = a1 * b0 + ((a0 * b0) >> 32);
  y1 = t;
  y2 = t >> 32;
  t = a0 * b1 + y1;
  y1 = t;
  t = a1 * b1 + y2 + (t >> 32);
  y2 = t;
  y3 = t >> 32;
  return ((uint64_t)y3 << 32) | y2;
}

inline int64_t mulh_helper(int64_t a, int64_t b) {
  int negate = (a < 0) != (b < 0);
  uint64_t res = mulhu_helper(a < 0 ? -a : a, b < 0 ? -b : b);
  return negate ? ~res + (a * b == 0) : res;
}

inline int64_t mulhsu_helper(int64_t a, uint64_t b) {
  int negate = a < 0;
  uint64_t res = mulhu_helper(a < 0 ? -a : a, b);
  return negate ? ~res + (a * b == 0) : res;
}

#define F32_SIGN ((uint32_t)1 << 31)
#define F64_SIGN ((uint64_t)1 << 63)

inline uint32_t fsgnj32(uint32_t a, uint32_t b, bool n, bool x) {
  uint32_t v = x ? a : n ? F32_SIGN : 0;
  return (a & ~F32_SIGN) | ((v ^ b) & F32_SIGN);
}

inline uint64_t fsgnj64(uint64_t a, uint64_t b, bool n, bool x) {
  uint64_t v = x ? a : n ? F64_SIGN : 0;
  return (a & ~F64_SIGN) | ((v ^ b) & F64_SIGN);
}

union u32_t_f32 {
  uint32_t ui;
  float f;
};
#define signF32UI(a) ((bool)((uint32_t)(a) >> 31))
#define expF32UI(a) ((int_fast16_t)((a) >> 23) & 0xFF)
#define fracF32UI(a) ((a)&0x007FFFFF)
#define isNaNF32UI(a) (((~(a)&0x7F800000) == 0) && ((a)&0x007FFFFF))
#define isSigNaNF32UI(uiA)                                                     \
  ((((uiA)&0x7FC00000) == 0x7F800000) && ((uiA)&0x003FFFFF))

inline uint16_t f32_classify(float a) {
  union u32_t_f32 uA;
  uint32_t uiA;

  uA.f = a;
  uiA = uA.ui;

  uint16_t infOrNaN = expF32UI(uiA) == 0xFF;
  uint16_t subnormalOrZero = expF32UI(uiA) == 0;
  bool sign = signF32UI(uiA);
  bool fracZero = fracF32UI(uiA) == 0;
  bool isNaN = isNaNF32UI(uiA);
  bool isSNaN = isSigNaNF32UI(uiA);

  return (sign && infOrNaN && fracZero) << 0 |
         (sign && !infOrNaN && !subnormalOrZero) << 1 |
         (sign && subnormalOrZero && !fracZero) << 2 |
         (sign && subnormalOrZero && fracZero) << 3 |
         (!sign && infOrNaN && fracZero) << 7 |
         (!sign && !infOrNaN && !subnormalOrZero) << 6 |
         (!sign && subnormalOrZero && !fracZero) << 5 |
         (!sign && subnormalOrZero && fracZero) << 4 | (isNaN && isSNaN) << 8 |
         (isNaN && !isSNaN) << 9;
}

union u64_t_f64 {
  uint64_t ui;
  double f;
};
#define signF64UI(a) ((bool)((uint64_t)(a) >> 63))
#define expF64UI(a) ((int_fast16_t)((a) >> 52) & 0x7FF)
#define fracF64UI(a) ((a)&UINT64_C(0x000FFFFFFFFFFFFF))
#define isNaNF64UI(a)                                                          \
  (((~(a)&UINT64_C(0x7FF0000000000000)) == 0) &&                               \
   ((a)&UINT64_C(0x000FFFFFFFFFFFFF)))
#define isSigNaNF64UI(uiA)                                                     \
  ((((uiA)&UINT64_C(0x7FF8000000000000)) == UINT64_C(0x7FF0000000000000)) &&   \
   ((uiA)&UINT64_C(0x0007FFFFFFFFFFFF)))

inline uint16_t f64_classify(double a) {
  union u64_t_f64 uA;
  uint64_t uiA;

  uA.f = a;
  uiA = uA.ui;

  uint16_t infOrNaN = expF64UI(uiA) == 0x7FF;
  uint16_t subnormalOrZero = expF64UI(uiA) == 0;
  bool sign = signF64UI(uiA);
  bool fracZero = fracF64UI(uiA) == 0;
  bool isNaN = isNaNF64UI(uiA);
  bool isSNaN = isSigNaNF64UI(uiA);

  return (sign && infOrNaN && fracZero) << 0 |
         (sign && !infOrNaN && !subnormalOrZero) << 1 |
         (sign && subnormalOrZero && !fracZero) << 2 |
         (sign && subnormalOrZero && fracZero) << 3 |
         (!sign && infOrNaN && fracZero) << 7 |
         (!sign && !infOrNaN && !subnormalOrZero) << 6 |
         (!sign && subnormalOrZero && !fracZero) << 5 |
         (!sign && subnormalOrZero && fracZero) << 4 | (isNaN && isSNaN) << 8 |
         (isNaN && !isSNaN) << 9;
}

#endif
