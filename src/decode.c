/**
 * @file decode.c
 * @brief
 */

#include "rvemu.h"

#define QUADRANT(data) (((data) >> 0) & 0x3)

/**
 * @brief decode the inst
 *
 * @param inst
 * @param data
 */
void inst_decode(inst_t *inst, uint32_t data) {
  uint32_t quadrand = QUADRANT(data);
  switch (quadrand) {
    case 0x0: panic("unimplemented");
    case 0x1: panic("unimplemented");
    case 0x2: panic("unimplemented");
    case 0x3: panic("unimplemented");
    default: unreachable();
  }
}
