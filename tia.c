#include "tia.h"
#include <string.h>

TIA_t tia;

void tia_init(TIA_t *tia) { memset(tia, 0, sizeof(TIA_t)); }

void tia_write(TIA_t *tia, uint8_t addr, uint8_t value) {
  switch (addr) {
  case 0x06:
    tia->COLUP0 = value;
    break;
  case 0x07:
    tia->COLUP1 = value;
    break;
  case 0x08:
    tia->COLUPF = value;
    break;
  case 0x1B:
    tia->GRP0 = value;
    break;
  case 0x1C:
    tia->GRP1 = value;
    break;
  case 0x1D:
    tia->ENAM0 = value;
    break;
  case 0x1E:
    tia->ENAM1 = value;
    break;
  case 0x1F:
    tia->ENABL = value;
    break;
  default:
    break;
  }
}

uint8_t tia_read(TIA_t *tia, uint8_t addr) {
  (void)tia;
  (void)addr;
  return 0; // Simplified
}

void tia_tick(TIA_t *tia) {
  tia->cycle++;
  if (tia->cycle >= 228) {
    tia->cycle = 0;
    tia->scanline++;
    if (tia->scanline >= 262) {
      tia->scanline = 0;
    }
  }
}
