#include "debug.h"
#include <stdio.h>

void debug_update(uint16_t pc, uint8_t opcode, uint8_t acc,
                  uint8_t x, uint8_t y, uint8_t sp,
                  uint8_t status, int cycles) {
  printf("PC:%04X OP:%02X A:%02X X:%02X Y:%02X SP:%02X ST:%02X CYC:%d\n", pc, opcode,
         acc, x, y, sp, status, cycles);
}
