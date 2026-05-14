#include <stdio.h>
#include "debug.h"

void debug_update(unsigned short pc, unsigned char opcode, unsigned char acc, unsigned char x, unsigned char y, unsigned char sp, unsigned char status) {
    printf("PC:%04X OP:%02X A:%02X X:%02X Y:%02X SP:%02X ST:%02X\n", 
           pc, opcode, acc, x, y, sp, status);
}
