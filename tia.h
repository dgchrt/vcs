#ifndef TIA_H
#define TIA_H

#include <stdint.h>

typedef struct {
    // Hardware Registers (Write-only from CPU)
    uint8_t COLUP0;  // Color Player 0
    uint8_t COLUP1;  // Color Player 1
    uint8_t COLUPF;  // Color Playfield
    uint8_t GRP0;    // Graphics Player 0
    uint8_t GRP1;    // Graphics Player 1
    uint8_t ENAM0;   // Enable Missile 0
    uint8_t ENAM1;   // Enable Missile 1
    uint8_t ENABL;   // Enable Ball

    // Internal State
    int scanline;    // 0 to 261
    int cycle;       // 0 to 227 (Clock cycles per line)
    uint32_t frame[160 * 192];
    
} TIA_t;

extern TIA_t tia;

void tia_init(TIA_t* tia);
void tia_write(TIA_t* tia, uint8_t addr, uint8_t value);
uint8_t tia_read(TIA_t* tia, uint8_t addr);
void tia_tick(TIA_t* tia);

#endif
