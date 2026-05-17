#include <stdint.h>

#ifndef DEBUG_H
#define DEBUG_H

void debug_update(uint16_t pc, uint8_t opcode, uint8_t acc,
                  uint8_t x, uint8_t y, uint8_t sp,
                  uint8_t status, int cycles);

#endif /* DEBUG_H */
